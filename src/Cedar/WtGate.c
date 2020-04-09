﻿// WideTunnel Source Code
// 
// Copyright (C) 2004-2017 SoftEther Corporation.
// All Rights Reserved.
// 
// http://www.softether.co.jp/
// Author: Daiyuu Nobori

// WtGate.c
// WideTunnel Gate

#include "CedarPch.h"

// Gate のセッションメイン関数
void WtgSessionMain(TSESSION *s)
{
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

#ifdef	OS_WIN32
	MsSetThreadPriorityRealtime();
#endif  // OS_WIN32

	WideGateReportSessionAdd(s->wt->Wide, s);

	Debug("WtgSessionMain Start.\n");

	SetSockEvent(s->SockEvent);

	while (true)
	{
		bool disconnected = false;

		// ソケットイベントを待機
		WtgWaitForSock(s);

		Lock(s->Lock);
		{
			do
			{
				s->StateChangedFlag = false;

				// クライアントからのデータを受信して処理
				WtgRecvFromClient(s);

				// サーバーからのデータを受信して処理
				WtgRecvFromServer(s);

				// クライアントへデータを送信
				WtgSendToClient(s);

				// サーバーへデータを送信
				WtgSendToServer(s);

				// TCP コネクションの切断の検査
				disconnected = WtgCheckDisconnect(s);

				if (s->Halt)
				{
					disconnected = true;
				}

				if (disconnected)
				{
					break;
				}
			}
			while (s->StateChangedFlag);
		}
		Unlock(s->Lock);

		if (disconnected)
		{
			// サーバーとの接続が切断されたのでセッションを終了する
			break;
		}
	}

	Debug("WtgSessionMain Cleanup...\n");

	WideGateReportSessionDel(s->wt->Wide, s->SessionId);

	// すべてのクライアントセッションの切断
	WtgDisconnectAllClientSession(s);

	Debug("WtgSessionMain End.\n");
}

// すべてのクライアントセッションの切断
void WtgDisconnectAllClientSession(TSESSION *s)
{
	UINT i;
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	for (i = 0;i < LIST_NUM(s->TunnelList);i++)
	{
		TUNNEL *t = LIST_DATA(s->TunnelList, i);

		WtDisconnectTTcp(t->ClientTcp);
	}
}

// TCP コネクションの切断の検査
bool WtgCheckDisconnect(TSESSION *s)
{
	UINT i;
	bool ret = false;
	LIST *o = NULL;
	// 引数チェック
	if (s == NULL)
	{
		return false;
	}

	for (i = 0;i < LIST_NUM(s->TunnelList);i++)
	{
		TUNNEL *t = LIST_DATA(s->TunnelList, i);

		if (t->ClientTcp->DisconnectSignalReceived)
		{
			// サーバー側から受け取った切断信号に対応するクライアントとの
			// コネクションは切断しなければならない
			t->ClientTcp->Disconnected = true;
		}

		if (WtIsTTcpDisconnected(s, t->ClientTcp))
		{
			// クライアントとの接続が切断された
			if (o == NULL)
			{
				o = NewListFast(NULL);
			}

			Add(o, t);
		}
	}

	if (o != NULL)
	{
		// 切断されたクライアントとの間のトンネルの解放
		for (i = 0;i < LIST_NUM(o);i++)
		{
			UINT tunnel_id;
			TUNNEL *t = LIST_DATA(o, i);

			tunnel_id = t->TunnelId;

//			Debug("Disconnect Tunnel: %u, time: %I64u\n", tunnel_id, SystemTime64());

			Delete(s->TunnelList, t);

			// トンネルの解放
			WtFreeTunnel(t);

			// サーバーに対して切断された旨の通知を送信
			WtInsertNewBlockToQueue(s->BlockQueue, s->ServerTcp,
				tunnel_id, NULL, 0);

			// トンネル ID を使用済みリストに追加
			WtAddUsedTunnelId(s->UsedTunnelList, tunnel_id, WT_TUNNEL_USED_EXPIRES * 2);
		}

		ReleaseList(o);
	}

	if (WtIsTTcpDisconnected(s, s->ServerTcp))
	{
		// サーバーとの接続が切断された
		ret = true;
	}

	return ret;
}

// 指定された TCP コネクションが切断されているかどうかチェックする
bool WtIsTTcpDisconnected(TSESSION *s, TTCP *ttcp)
{
	// 引数チェック
	if (ttcp == NULL || s == NULL)
	{
		return true;
	}

	if (ttcp->Disconnected == false)
	{
		if ((ttcp->LastCommTime + (UINT64)WT_TUNNEL_TIMEOUT) < s->Tick)
		{
			ttcp->Disconnected = true;
		}
	}

	if (ttcp->Disconnected)
	{
		Disconnect(ttcp->Sock);

		return true;
	}

	return false;
}

// サーバーへデータを送信
void WtgSendToServer(TSESSION *s)
{
	TTCP *ttcp;
	QUEUE *blockqueue;
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	ttcp = s->ServerTcp;
	blockqueue = s->BlockQueue;

	// 送信データの生成
	WtMakeSendDataTTcp(s, ttcp, blockqueue);

	// 送信
	WtSendTTcp(s, ttcp);
}

// クライアントへデータを送信
void WtgSendToClient(TSESSION *s)
{
	UINT i;
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	for (i = 0;i < LIST_NUM(s->TunnelList);i++)
	{
		TUNNEL *t = LIST_DATA(s->TunnelList, i);
		TTCP *ttcp = t->ClientTcp;
		QUEUE *blockqueue = t->BlockQueue;

		// 送信データの生成
		WtMakeSendDataTTcp(s, ttcp, blockqueue);

		// 送信
		WtSendTTcp(s, ttcp);
	}
}

// 送信データの生成
void WtMakeSendDataTTcp(TSESSION *s, TTCP *ttcp, QUEUE *blockqueue)
{
	DATABLOCK *block;
	FIFO *fifo;
	UINT i;
	// 引数チェック
	if (s == NULL || ttcp == NULL || blockqueue == NULL)
	{
		return;
	}

	fifo = ttcp->SendFifo;

	while ((block = GetNext(blockqueue)) != NULL)
	{
		if (ttcp->MultiplexMode)
		{
			i = Endian32(block->TunnelId);
			WriteFifo(fifo, &i, sizeof(UINT));
		}

		i = Endian32(block->PhysicalSize);
		WriteFifo(fifo, &i, sizeof(UINT));
		WriteFifo(fifo, block->Data, block->PhysicalSize);

		if (block->DataSize == 0)
		{
			ttcp->DisconnectSignalReceived = true;
		}

		WtFreeDataBlock(block, false);

		ttcp->LastKeepAliveTime = s->Tick;
	}

	if ((ttcp->LastKeepAliveTime + (UINT64)WT_TUNNEL_KEEPALIVE) < s->Tick)
	{
		i = Endian32(0);

		WriteFifo(fifo, &i, sizeof(UINT));

		ttcp->LastKeepAliveTime = s->Tick;
	}
}

// サーバーからのデータを受信
void WtgRecvFromServer(TSESSION *s)
{
	TTCP *ttcp;
	QUEUE *q;
	DATABLOCK *block;
	UINT last_tid = INFINITE;
	UINT i;
	UINT max_fifo_size = 0;
	UINT remain_buf_size = WT_WINDOW_SIZE;
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	ttcp = s->ServerTcp;

	for (i = 0;i < LIST_NUM(s->TunnelList);i++)
	{
		TUNNEL *t = LIST_DATA(s->TunnelList, i);

		max_fifo_size = MAX(max_fifo_size, FifoSize(t->ClientTcp->SendFifo));
	}

	if (WT_WINDOW_SIZE > max_fifo_size)
	{
		remain_buf_size = WT_WINDOW_SIZE - max_fifo_size;
	}
	else
	{
		remain_buf_size = 0;
	}

	// Debug("remain_buf_size: %u\n", remain_buf_size);

	// TTCP からデータを受信
	WtRecvTTcpEx(s, ttcp, remain_buf_size);

	// 受信データを解釈
	q = WtParseRecvTTcp(s, ttcp);

	// 受信データをクライアントに対して配布
	while ((block = GetNext(q)) != NULL)
	{
		UINT tunnel_id = block->TunnelId;
		TUNNEL *t = WtgSearchTunnelById(s->TunnelList, tunnel_id);
		QUEUE *dest_queue;
		DATABLOCK *send_block;
		bool use_compress = false;

		if (t != NULL)
		{
			// クライアントに対してデータを転送する
			dest_queue = t->BlockQueue;
			send_block = block;
			use_compress = t->ClientTcp->UseCompress;
		}
		else
		{
			if (block->DataSize == 0)
			{
				// 存在しないクライアントに対して切断指令が送信されようとした
				// ので無視する
				WtFreeDataBlock(block, false);
				continue;
			}

			// 存在しないクライアントに対してデータが送信されようとした
			// のでサーバーに対して切断通知を送信する
			WtFreeDataBlock(block, false);

			if (tunnel_id != last_tid)
			{
				last_tid = tunnel_id;
				send_block = WtNewDataBlock(tunnel_id, NULL, 0, 0);
				dest_queue = s->BlockQueue;
				use_compress = s->ServerTcp->UseCompress;
			}
			else
			{
				send_block = NULL;
			}
		}

		if (send_block != NULL)
		{
			send_block = WtRebuildDataBlock(send_block, use_compress ? 1 : 0);
			InsertQueue(dest_queue, send_block);
		}
	}

	ReleaseQueue(q);
}

// 新しいブロックを送信キューに追加
void WtInsertNewBlockToQueue(QUEUE *dest_queue, TTCP *dest_ttcp, UINT src_tunnel_id, void *data, UINT size)
{
	DATABLOCK *block;
	// 引数チェック
	if (dest_queue == NULL || dest_ttcp == NULL)
	{
		return;
	}

	block = WtNewDataBlock(src_tunnel_id, data, size, dest_ttcp->UseCompress ? 1 : 0);
	InsertQueue(dest_queue, block);
}

// データブロックの再構築 (古いデータブロックは解放)
DATABLOCK *WtRebuildDataBlock(DATABLOCK *src_block, int compress_flag)
{
	DATABLOCK *ret;
	// 引数チェック
	if (src_block == NULL)
	{
		return NULL;
	}

	if (compress_flag == 0)
	{
		// 無圧縮
		ret = src_block;
	}
	else
	{
		// 圧縮
		ret = WtNewDataBlock(src_block->TunnelId, src_block->Data, src_block->DataSize, compress_flag);
		WtFreeDataBlock(src_block, true);
	}

	return ret;
}

// クライアントからのデータを受信
void WtgRecvFromClient(TSESSION *s)
{
	UINT i;
	DATABLOCK *block;
	// 引数チェック 
	if (s == NULL)
	{
		return;
	}

	// Debug("FifoSize(s->ServerTcp->SendFifo): %u\n", FifoSize(s->ServerTcp->SendFifo));

	for (i = 0;i < LIST_NUM(s->TunnelList);i++)
	{
		TTCP *ttcp;
		TUNNEL *p = LIST_DATA(s->TunnelList, i);
		QUEUE *q;

		ttcp = p->ClientTcp;

		if (FifoSize(s->ServerTcp->SendFifo) > WT_WINDOW_SIZE)
		{
			// サーバー宛の FIFO にデータが溜まりすぎている場合は新たに
			// クライアントからのデータを受信しない

			// タイムアウト防止
			ttcp->LastCommTime = s->Tick;
			continue;
		}

		// TTCP からデータを受信
		WtRecvTTcp(s, ttcp);

		// 受信データを解釈
		q = WtParseRecvTTcp(s, ttcp);

		// 受信データをサーバーに転送
		while ((block = GetNext(q)) != NULL)
		{
			if (block->DataSize != 0)
			{
				UINT tunnel_id = p->TunnelId;
				QUEUE *dest_queue;
				DATABLOCK *send_block;
				bool use_compress;

				dest_queue = s->BlockQueue;
				use_compress = s->ServerTcp->UseCompress;
				send_block = WtRebuildDataBlock(block, use_compress ? 1 : 0);
				block->TunnelId = p->TunnelId;
				InsertQueue(dest_queue, send_block);
			}
			else
			{
				WtFreeDataBlock(block, false);
			}
		}

		ReleaseQueue(q);
	}
}

// 受信データを解釈
QUEUE *WtParseRecvTTcp(TSESSION *s, TTCP *ttcp)
{
	QUEUE *q;
	FIFO *fifo;
	// 引数チェック
	if (s == NULL || ttcp == NULL)
	{
		return NULL;
	}

	q = NewQueueFast();

	if (ttcp->WantSize == 0)
	{
		ttcp->WantSize = sizeof(UINT);
	}

	fifo = ttcp->RecvFifo;

	while (fifo->size >= ttcp->WantSize)
	{
		UCHAR *buf;
		void *data;
		DATABLOCK *block;
		UINT i;

		buf = (UCHAR *)fifo->p + fifo->pos;

		switch (ttcp->Mode)
		{
		case 0:
			// コネクション番号
			if (ttcp->MultiplexMode == false)
			{
				// 多重化モードでない場合は直接データサイズ受信に飛ぶ
				goto READ_DATA_SIZE;
			}

			ttcp->WantSize = sizeof(UINT);
			Copy(&i, buf, sizeof(UINT));
			ttcp->CurrentBlockConnectionId = Endian32(i);
			ReadFifo(fifo, NULL, sizeof(UINT));

			if (ttcp->CurrentBlockConnectionId != 0)
			{
				ttcp->Mode = 1;
			}
			else
			{
//				Debug("keep alive\n");
			}
			break;

		case 1:
READ_DATA_SIZE:
			// データサイズ
			Copy(&i, buf, sizeof(UINT));
			i = Endian32(i);

			if (i > WT_MAX_BLOCK_SIZE)
			{
				// 不正なデータサイズを受信。通信エラーか
				ttcp->Disconnected = true;
				ttcp->WantSize = sizeof(UINT);
				ReadFifo(fifo, NULL, sizeof(UINT));
				ttcp->Mode = 0;
			}
			else
			{
				ttcp->CurrentBlockSize = i;
				ReadFifo(fifo, NULL, sizeof(UINT));
				ttcp->WantSize = ttcp->CurrentBlockSize;
				ttcp->Mode = 2;
			}
			break;

		case 2:
			// データ本体
			data = Malloc(ttcp->CurrentBlockSize);
			Copy(data, buf, ttcp->CurrentBlockSize);
			ReadFifo(fifo, NULL, ttcp->CurrentBlockSize);

			block = WtNewDataBlock(ttcp->CurrentBlockConnectionId, data, ttcp->CurrentBlockSize,
				ttcp->UseCompress ? -1 : 0);

			InsertQueue(q, block);

			ttcp->WantSize = sizeof(UINT);
			ttcp->Mode = 0;
			break;
		}
	}

	return q;
}

// データブロックの解放
void WtFreeDataBlock(DATABLOCK *block, bool no_free_data)
{
	// 引数チェック
	if (block == NULL)
	{
		return;
	}

	if (no_free_data == false)
	{
		Free(block->Data);
	}

	Free(block);
}

// 新しいデータブロックの作成
DATABLOCK *WtNewDataBlock(UINT tunnel_id, void *data, UINT size, int compress_flag)
{
	DATABLOCK *block;

	if (size == 0 && data == NULL)
	{
		data = Malloc(1);
	}

	block = ZeroMalloc(sizeof(DATABLOCK));

	if (compress_flag == 0)
	{
		// 無圧縮
		block->Compressed = false;
		block->Data = data;
		block->DataSize = block->PhysicalSize = size;
		block->TunnelId = tunnel_id;
	}
	else if (compress_flag > 0)
	{
		UINT max_size;

		// 圧縮
		block->Compressed = true;
		max_size = CalcCompress(size);
		block->Data = Malloc(max_size);
		block->PhysicalSize = Compress(block->Data, max_size, data, size);
		block->DataSize = size;

		Free(data);
	}
	else
	{
		UINT max_size = WT_MAX_BLOCK_SIZE;
		void *tmp;
		UINT sz;

		// 解凍
		tmp = Malloc(max_size);
		sz = Uncompress(tmp, max_size, data, size);
		Free(data);

		block->Data = Clone(tmp, sz);
		Free(tmp);
		block->PhysicalSize = block->DataSize = sz;
	}

	return block;
}

// TTCP にデータを送信
void WtSendTTcp(TSESSION *s, TTCP *ttcp)
{
	SOCK *sock;
	FIFO *fifo;
	// 引数チェック
	if (ttcp == NULL || s == NULL)
	{
		return;
	}
	if (ttcp->Disconnected)
	{
		return;
	}

	sock = ttcp->Sock;
	if (sock->AsyncMode == false)
	{
		return;
	}

	fifo = ttcp->SendFifo;

	while (fifo->size != 0)
	{
		UCHAR *buf;
		UINT want_send_size;
		UINT size;

		buf = (UCHAR *)fifo->p + fifo->pos;
		want_send_size = fifo->size;

		size = WtSendSock(ttcp, buf, want_send_size);
		if (size == 0)
		{
			// 切断された
			ttcp->Disconnected = true;
			ClearFifo(fifo);
			break;
		}
		else if (size == SOCK_LATER)
		{
			// 送信に時間がかかっている
			break;
		}
		else
		{
			// 送信完了
			ReadFifo(fifo, NULL, size);

			ttcp->LastCommTime = s->Tick;
			s->StateChangedFlag = true;
		}
	}
}

// TTCP からデータを受信
void WtRecvTTcp(TSESSION *s, TTCP *ttcp)
{
	WtRecvTTcpEx(s, ttcp, INFINITE);
}
void WtRecvTTcpEx(TSESSION *s, TTCP *ttcp, UINT remain_buf_size)
{
	SOCK *sock;
	UINT size;
	void *recvbuf = s->RecvBuf;
	// 引数チェック
	if (ttcp == NULL || s == NULL)
	{
		return;
	}
	if (ttcp->Disconnected)
	{
		return;
	}

	sock = ttcp->Sock;
	if (sock->Connected == false)
	{
		ttcp->Disconnected = true;
		return;
	}
	if (sock->AsyncMode == false)
	{
		return;
	}

	// 受信
RECV_START:
	if (remain_buf_size == 0)
	{
		ttcp->LastCommTime = s->Tick;
		return;
	}
	size = WtRecvSock(ttcp, recvbuf, MIN(RECV_BUF_SIZE, remain_buf_size));
	if (size == 0)
	{
TTCP_DISCONNECTED:
		// コネクションが切断された
		ttcp->Disconnected = true;
		return;
	}
	else if (size == SOCK_LATER)
	{
		// 受信待ち
		if ((s->Tick > ttcp->LastCommTime) && ((s->Tick - ttcp->LastCommTime) >= (UINT64)WT_TUNNEL_TIMEOUT))
		{
			// タイムアウト発生
			goto TTCP_DISCONNECTED;
		}
	}
	else
	{
		// データを受信
		ttcp->LastCommTime = s->Tick;
		// s->StateChangedFlag = true;

		WriteFifo(ttcp->RecvFifo, recvbuf, size);
		remain_buf_size -= size;

		goto RECV_START;
	}
}

// データの受信
UINT WtRecvSock(TTCP *ttcp, void *buf, UINT size)
{
	// 引数チェック
	if (ttcp == NULL || ttcp->Sock == NULL)
	{
		return 0;
	}

	return Recv(ttcp->Sock, buf, size, ttcp->Sock->SecureMode);
}

// データの送信
UINT WtSendSock(TTCP *ttcp, void *buf, UINT size)
{
	// 引数チェック
	if (ttcp == NULL || ttcp->Sock == NULL)
	{
		return 0;
	}

	return Send(ttcp->Sock, buf, size, ttcp->Sock->SecureMode);
}

// ソケットイベントを待機
void WtgWaitForSock(TSESSION *s)
{
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	Lock(s->Lock);
	{
		UINT i;
		SOCK *sock = s->ServerTcp->Sock;

		JoinSockToSockEvent(sock, s->SockEvent);

		for (i = 0;i < LIST_NUM(s->TunnelList);i++)
		{
			TUNNEL *p = LIST_DATA(s->TunnelList, i);
			SOCK *sock = p->ClientTcp->Sock;

			JoinSockToSockEvent(sock, s->SockEvent);
		}
	}
	Unlock(s->Lock);

	WaitSockEvent(s->SockEvent, SELECT_TIME);

	s->Tick = Tick64();
}

// Gate による接続受付
void WtgAccept(WT *wt, SOCK *s)
{
	PACK *p;
	UCHAR session_id[WT_SESSION_ID_SIZE];
	WT_GATE_CONNECT_PARAM param;
	UINT code;
	char method[128];
	bool use_compress = false;
	TSESSION *session = NULL;
	char ip_str[MAX_PATH];
	// 引数チェック
	if (wt == NULL || s == NULL)
	{
		return;
	}

	IPToStr(ip_str, sizeof(ip_str), &s->RemoteIP);

	Debug("WtgAccept() from %s\n", ip_str);

	SetTimeout(s, CONNECTING_TIMEOUT);

	//SetSocketSendRecvBufferSize((int)s, WT_SOCKET_WINDOW_SIZE);

	// セッション ID の生成
	Rand(session_id, sizeof(session_id));

	SetWantToUseCipher(s, "RC4-MD5");

	// SSL 通信の開始
	if (StartSSLEx(s, wt->GateCert, wt->GateKey, true, 0, WT_SNI_STRING_V2) == false)
	{
		Debug("StartSSL Failed.\n");
		return;
	}

	// シグネチャのダウンロード
	if (WtgDownloadSignature(s) == false)
	{
		Debug("WtgDownloadSignature Failed.\n");
		return;
	}

	// Hello パケットのアップロード
	if (WtgUploadHello(wt, s, session_id) == false)
	{
		Debug("WtgUploadHello Failed.\n");
		WtgSendError(s, ERR_PROTOCOL_ERROR);
		return;
	}

	// 接続パラメータのダウンロード
	p = HttpServerRecv(s);
	if (p == NULL)
	{
		Debug("HttpServerRecv Failed.\n");
		WtgSendError(s, ERR_PROTOCOL_ERROR);
		return;
	}

	if (PackGetStr(p, "method", method, sizeof(method)) == false)
	{
		FreePack(p);
		WtgSendError(s, ERR_PROTOCOL_ERROR);
		return;
	}

	Debug("method: %s\n", method);
	if (StrCmpi(method, "new_session") == 0)
	{
		bool request_initial_pack;
		UINT64 server_mask_64 = 0;
		// 新しいセッションの確立
		Zero(&param, sizeof(param));
		if (WtGateConnectParamFromPack(&param, p) == false)
		{
			Debug("WtGateConnectParamFromPack failed.\n");
			FreePack(p);
			return;
		}

		// パラメータの取得
		use_compress = PackGetBool(p, "use_compress");
		request_initial_pack = PackGetBool(p, "request_initial_pack");
		server_mask_64 = PackGetInt64(p, "server_mask_64");

		FreePack(p);

		// 接続パラメータの電子署名のチェック
		if (WtGateConnectParamCheckSignature(&param) == false)
		{
			FreeX(param.Cert);
			WtgSendError(s, ERR_PROTOCOL_ERROR);
			Debug("WtGateConnectParamCheckSignature Failed.\n");
			return;
		}

		// 接続パラメータの証明書が信頼できるかどうかチェック
		if (WtIsTrustedCert(wt, param.Cert) == false)
		{
			FreeX(param.Cert);
			WtgSendError(s, ERR_PROTOCOL_ERROR);
			Debug("WtIsTrustedCert Failed.\n");
			return;
		}

		// GateID のチェック
		if (Cmp(wt->GateId, param.GateId, SHA1_SIZE) != 0)
		{
			FreeX(param.Cert);
			WtgSendError(s, ERR_PROTOCOL_ERROR);
			Debug("Cmp GateID Failed.\n");
			return;
		}

		// 有効期限のチェック
		if (param.Expires < SystemTime64())
		{
			FreeX(param.Cert);
			WtgSendError(s, ERR_PROTOCOL_ERROR);
			Debug("Expires Failed.\n");
			return;
		}

		FreeX(param.Cert);
		param.Cert = NULL;

		code = ERR_NO_ERROR;

		LockList(wt->SessionList);
		{
			// 既に同一の MSID を持つセッションが存在しないかどうか確認
			UINT i;
			bool exists = false;

			if (false)
			{
				for (i = 0;i < LIST_NUM(wt->SessionList);i++)
				{
					TSESSION *s = LIST_DATA(wt->SessionList, i);

					if (StrCmpi(s->Msid, param.Msid) == 0)
					{
						// 同一の MSID を持ったセッションを発見
						//exists = true;
						// 同一の MSID を持ったセッションが存在しても構わず接続させる
						break;
					}
				}
			}

			if (exists == false)
			{
				// セッションの作成
				TSESSION *sess = WtgNewSession(wt, s, param.Msid, session_id, use_compress, request_initial_pack);

				sess->ServerMask64 = server_mask_64;

				Insert(wt->SessionList, sess);

				session = sess;
			}
			else
			{
				// すでに接続されている
				code = ERR_MACHINE_ALREADY_CONNECTED;
				Debug("Error: ERR_MACHINE_ALREADY_CONNECTED.\n");
			}
		}
		UnlockList(wt->SessionList);

		if (code != ERR_NO_ERROR)
		{
			// セッションの確立に失敗
			WtgSendError(s, code);
			return;
		}

		// 接続成功。
		p = NewPack();
		PackAddInt(p, "code", ERR_NO_ERROR);
		HttpServerSend(s, p);
		FreePack(p);

		SetTimeout(s, TIMEOUT_INFINITE);

		// セッションメイン
		WtgSessionMain(session);

		LockList(wt->SessionList);
		{
			// セッションの削除
			Delete(wt->SessionList, session);
		}
		UnlockList(wt->SessionList);

		WtReleaseSession(session);
	}
	else if (StrCmpi(method, "connect_session") == 0)
	{
		// 既存のセッションへの接続
		char session_id[WT_SESSION_ID_SIZE];
		UCHAR client_id[SHA1_SIZE];
		TSESSION *session = NULL;

		Zero(client_id, sizeof(client_id));

		// パラメータの取得
		use_compress = PackGetBool(p, "use_compress");

		Zero(session_id, sizeof(session_id));
		PackGetData2(p, "session_id", session_id, sizeof(session_id));

		PackGetData2(p, "rand", rand, SHA1_SIZE);

		// Client ID
		PackGetData2(p, "client_id", client_id, sizeof(client_id));
		if (IsZero(client_id, sizeof(client_id)))
		{
			// Client ID を IP アドレスから生成
			WtGenerateClientIdFromIP(client_id, &s->RemoteIP);
		}

		FreePack(p);

		// セッションの検索
		LockList(wt->SessionList);
		{
			if (true)
			{
				// 正規動作: セッションの検索
				TSESSION t;

				Zero(&t, sizeof(t));
				Copy(t.SessionId, session_id, WT_SESSION_ID_SIZE);

				session = Search(wt->SessionList, &t);

				if (session != NULL)
				{
					AddRef(session->Ref);
				}
			}
			else
			{
				// デバッグ動作: 1 番目のセッションを選択
				if (LIST_NUM(wt->SessionList) >= 1)
				{
					session = LIST_DATA(wt->SessionList, 0);
					AddRef(session->Ref);
				}
			}
		}
		UnlockList(wt->SessionList);

		if (session == NULL)
		{
			// 指定されたセッション ID は存在しない
			WtgSendError(s, ERR_DEST_MACHINE_NOT_EXISTS);
			Debug("Error: ERR_DEST_MACHINE_NOT_EXISTS\n");
			return;
		}

		// 接続成功。

		p = NewPack();
		PackAddInt(p, "code", ERR_NO_ERROR);
		HttpServerSend(s, p);
		FreePack(p);

		SetTimeout(s, TIMEOUT_INFINITE);

		Lock(session->Lock);
		{
			// 新しいトンネルの生成
			UINT tunnel_id = WtgGenerateNewTunnelId(session);
			TUNNEL *tunnel;
			TTCP *ttcp;

			Debug("New Tunnel: %u\n", tunnel_id);

			ttcp = WtNewTTcp(s, use_compress);

			tunnel = WtNewTunnel(ttcp, tunnel_id, NULL);
			Copy(tunnel->ClientId, client_id, sizeof(client_id));
			Insert(session->TunnelList, tunnel);

			// Initial Pack の作成
			if (session->RequestInitialPack)
			{
				PACK *p = NewPack();
				BUF *b;
				UCHAR *buffer;

				PackAddIp(p, "ClientIP", &s->RemoteIP);
				PackAddInt(p, "ClientPort", s->RemotePort);
				PackAddStr(p, "ClientHost", s->RemoteHostname);
				PackAddIp(p, "GateIP", &s->LocalIP);
				PackAddInt(p, "GatePort", s->LocalPort);
				PackAddInt64(p, "ClientConnectedTime", SystemTime64());
				PackAddInt(p, "TunnelId", tunnel_id);
				PackAddData(p, "ClientID", tunnel->ClientId, sizeof(tunnel->ClientId));

				b = PackToBuf(p);
				FreePack(p);

				buffer = ZeroMalloc(WT_INITIAL_PACK_SIZE);
				Copy(buffer, b->Buf, MIN(b->Size, WT_INITIAL_PACK_SIZE));
				FreeBuf(b);

				if (true)
				{
					DATABLOCK *block = WtNewDataBlock(tunnel_id, buffer, WT_INITIAL_PACK_SIZE,
						session->ServerTcp->UseCompress ? 1 : 0);
					block->TunnelId = tunnel_id;

					InsertQueue(session->BlockQueue, block);
				}
			}
		}
		Unlock(session->Lock);

		SetSockEvent(session->SockEvent);

		WtReleaseSession(session);
	}
}

// クライアント ID を IP アドレスから生成する
void WtGenerateClientIdFromIP(UCHAR *client_id, IP *ip)
{
	char ipstr[MAX_PATH];
	// 引数チェック
	if (client_id == NULL || ip == NULL)
	{
		return;
	}

	IPToStr(ipstr, sizeof(ipstr), ip);

	HashSha1(client_id, ipstr, StrLen(ipstr));
}

// 新しいトンネル ID の決定
UINT WtgGenerateNewTunnelId(TSESSION *s)
{
	UINT id = Rand32();
	LIST *o;
	UINT i = 0;
	// 引数チェック
	if (s == NULL)
	{
		return 0;
	}

	o = s->TunnelList;

	while (true)
	{
		TUNNEL *t;

		while (true)
		{
			id = Rand32();
			if (id != 0 && id != INFINITE)
			{
				break;
			}
		}

		if (WtIsTunnelIdExistsInUsedTunnelIdList(s->UsedTunnelList, id) == false)
		{
			t = WtgSearchTunnelById(o, id);
			if (t == NULL)
			{
				break;
			}
		}
	}

	return id;
}

// 新しいトンネルの作成
TUNNEL *WtNewTunnel(TTCP *client_tcp, UINT tunnel_id, SOCKIO *sockio)
{
	TUNNEL *p;

	p = ZeroMalloc(sizeof(TUNNEL));
	p->BlockQueue = NewQueue();
	p->ClientTcp = client_tcp;
	p->TunnelId = tunnel_id;

	if (sockio != NULL)
	{
		p->SockIo = sockio;
		AddRef(sockio->Ref);
	}

	return p;
}

// Gate 上のセッションの作成
TSESSION *WtgNewSession(WT *wt, SOCK *sock, char *msid, void *session_id, bool use_compress, bool request_initial_pack)
{
	TSESSION *s;
	// 引数チェック
	if (msid == NULL || session_id == NULL || sock == NULL)
	{
		return NULL;
	}

	s = ZeroMalloc(sizeof(TSESSION));
	s->Lock = NewLock();
	s->Ref = NewRef();
	s->SessionType = WT_SESSION_GATE;
	StrCpy(s->Msid, sizeof(s->Msid), msid);
	Copy(s->SessionId, session_id, WT_SESSION_ID_SIZE);
	s->EstablishedTick = Tick64();
	s->ServerTcp = WtNewTTcp(sock, use_compress);
	s->ServerTcp->MultiplexMode = true;
	s->BlockQueue = NewQueue();
	s->SockEvent = NewSockEvent();
	s->TunnelList = NewList(WtgCompareTunnel);
	s->RecvBuf = Malloc(RECV_BUF_SIZE);
	s->UsedTunnelList = WtNewUsedTunnelIdList();
	s->RequestInitialPack = request_initial_pack;
	s->wt = wt;

	return s;
}

// セッションの解放
void WtReleaseSession(TSESSION *s)
{
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	if (Release(s->Ref) == 0)
	{
		WtCleanupSession(s);
	}
}

// セッションのクリーンアップ
void WtCleanupSession(TSESSION *s)
{
	UINT i;
	// 引数チェック
	if (s == NULL)
	{
		return;
	}

	WtFreeDataBlockQueue(s->BlockQueue);

	for (i = 0;i < LIST_NUM(s->TunnelList);i++)
	{
		TUNNEL *t = LIST_DATA(s->TunnelList, i);

		WtFreeTunnel(t);
	}

	ReleaseList(s->TunnelList);
	DeleteLock(s->Lock);
	ReleaseSockEvent(s->SockEvent);
	Free(s->RecvBuf);
	WtFreeTTcp(s->ServerTcp);

	Disconnect(s->Sock);
	ReleaseSock(s->Sock);
	ReleaseThread(s->ConnectThread);
	if (s->ConnectParam != NULL)
	{
		WtFreeConnect(s->ConnectParam);
		Free(s->ConnectParam);
	}

	ReleaseList(s->AcceptThreadList);

	WtFreeTTcp(s->GateTcp);

	WtFreeUsedTunnelIdList(s->UsedTunnelList);

	WtFreeTunnel(s->ClientTunnel);

	Free(s);
}

// データブロックの入ったキューを解放
void WtFreeDataBlockQueue(QUEUE *q)
{
	DATABLOCK *block;
	// 引数チェック
	if (q == NULL)
	{
		return;
	}

	while ((block = GetNext(q)) != NULL)
	{
		WtFreeDataBlock(block, false);
	}

	ReleaseQueue(q);
}

// トンネルの解放
void WtFreeTunnel(TUNNEL *t)
{
	// 引数チェック
	if (t == NULL)
	{
		return;
	}

	WtFreeDataBlockQueue(t->BlockQueue);
	WtFreeTTcp(t->ClientTcp);
	SockIoDisconnect(t->SockIo);
	ReleaseSockIo(t->SockIo);

	Free(t);
}

// TTCP の解放
void WtFreeTTcp(TTCP *ttcp)
{
	// 引数チェック
	if (ttcp == NULL)
	{
		return;
	}

	Disconnect(ttcp->Sock);
	ReleaseSock(ttcp->Sock);
	ReleaseFifo(ttcp->SendFifo);
	ReleaseFifo(ttcp->RecvFifo);

	Free(ttcp);
}

// TTCP の作成
TTCP *WtNewTTcp(SOCK *s, bool use_compress)
{
	TTCP *t;
	// 引数チェック
	if (s == NULL)
	{
		return NULL;
	}

	t = ZeroMalloc(sizeof(TTCP));
	t->Sock = s;
	StrCpy(t->Hostname, sizeof(t->Hostname), s->RemoteHostname);
	Copy(&t->Ip, &s->RemoteIP, sizeof(IP));
	t->Port = s->RemotePort;
	t->LastCommTime = Tick64();
	t->RecvFifo = NewFifo();
	t->SendFifo = NewFifo();
	t->UseCompress = use_compress;
	AddRef(s->ref);

	return t;
}

// トンネルリストからトンネルの取得
TUNNEL *WtgSearchTunnelById(LIST *o, UINT id)
{
	TUNNEL t, *ret;
	// 引数チェック
	if (o == NULL || id == 0)
	{
		return NULL;
	}

	Zero(&t, sizeof(t));
	t.TunnelId = id;

	ret = Search(o, &t);

	return ret;
}

// トンネルリストの比較
int WtgCompareTunnel(void *p1, void *p2)
{
	TUNNEL *t1, *t2;
	if (p1 == NULL || p2 == NULL)
	{
		return 0;
	}
	t1 = *(TUNNEL **)p1;
	t2 = *(TUNNEL **)p2;
	if (t1 == NULL || t2 == NULL)
	{
		return 0;
	}

	if (t1->TunnelId > t2->TunnelId)
	{
		return 1;
	}
	else if (t1->TunnelId < t2->TunnelId)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

// セッションリストの比較
int WtgCompareSession(void *p1, void *p2)
{
	TSESSION *s1, *s2;
	if (p1 == NULL || p2 == NULL)
	{
		return 0;
	}
	s1 = *(TSESSION **)p1;
	s2 = *(TSESSION **)p2;
	if (s1 == NULL || s2 == NULL)
	{
		return 0;
	}

	return Cmp(s1->SessionId, s2->SessionId, WT_SESSION_ID_SIZE);
}

// エラーの送信
bool WtgSendError(SOCK *s, UINT code)
{
	PACK *p;
	bool ret;
	// 引数チェック
	if (s == NULL)
	{
		return false;
	}

	p = NewPack();
	PackAddInt(p, "code", code);

	ret = HttpServerSend(s, p);
	FreePack(p);

	return ret;
}

// Hello パケットのアップロード
bool WtgUploadHello(WT *wt, SOCK *s, void *session_id)
{
	PACK *p;
	// 引数チェック
	if (wt == NULL || s == NULL || session_id == NULL)
	{
		return false;
	}

	p = NewPack();
	PackAddData(p, "session_id", session_id, WT_SESSION_ID_SIZE);
	PackAddInt(p, "hello", 1);

	if(HttpServerSend(s, p) == false)
	{
		FreePack(p);
		return false;
	}

	FreePack(p);

	return true;
}

// シグネチャのダウンロード
bool WtgDownloadSignature(SOCK *s)
{
	HTTP_HEADER *h;
	UCHAR *data;
	UINT data_size;
	UINT num = 0, max = 19;
	// 引数チェック
	if (s == NULL)
	{
		return false;
	}

	while (true)
	{
		num++;
		if (num > max)
		{
			// 切断
			return false;
		}
		// ヘッダを受信する
		h = RecvHttpHeader(s);
		if (h == NULL)
		{
			return false;
		}

		// 解釈する
		if (StrCmpi(h->Method, "POST") == 0)
		{
			// POST なのでデータを受信する
			data_size = GetContentLength(h);
			if ((data_size > MAX_WATERMARK_SIZE || data_size < SizeOfWaterMark()) && (data_size != StrLen(HTTP_WIDE_TARGET_POSTDATA)))
			{
				// データが大きすぎる
				HttpSendForbidden(s, h->Target, NULL);
				FreeHttpHeader(h);
				return false;
			}
			data = Malloc(data_size);
			if (RecvAll(s, data, data_size, s->SecureMode) == false)
			{
				// データ受信失敗
				Free(data);
				FreeHttpHeader(h);
				return false;
			}
			// Target を確認する
			if (StrCmpi(h->Target, HTTP_WIDE_TARGET2) != 0)
			{
				// ターゲットが不正
				HttpSendNotFound(s, h->Target);
				Free(data);
				FreeHttpHeader(h);
			}
			else
			{
				Free(data);
				FreeHttpHeader(h);
				return true;
			}
		}
		else
		{
			// これ以上解釈しても VPN クライアントで無い可能性が高いが
			// 一応する
			if (StrCmpi(h->Method, "GET") != 0)
			{
				// サポートされていないメソッド呼び出し
				HttpSendNotImplemented(s, h->Method, h->Target, h->Version);
			}
			else
			{
				if (StrCmpi(h->Target, "/") == 0)
				{
					// Free 版以外
					HttpSendForbidden(s, h->Target, NULL);
				}
				else
				{
					// Not Found
					HttpSendNotFound(s, h->Target);
				}
			}
			FreeHttpHeader(h);
		}
	}
}

// WT_GATE_CONNECT_PARAM の電子署名をチェック
bool WtGateConnectParamCheckSignature(WT_GATE_CONNECT_PARAM *g)
{
	BUF *b;
	K *k;
	bool ret;
	// 引数チェック
	if (g == NULL)
	{
		return false;
	}

	b = WtGateConnectParamPayloadToBuf(g);
	if (b == NULL)
	{
		return false;
	}

	k = GetKFromX(g->Cert);
	if (k == NULL)
	{
		FreeBuf(b);
		return false;
	}

	ret = RsaVerify(b->Buf, b->Size, g->Signature, k);

	FreeK(k);
	FreeBuf(b);

	return ret;
}

// WT_GATE_CONNECT_PARAM を Pack から取得
bool WtGateConnectParamFromPack(WT_GATE_CONNECT_PARAM *g, PACK *p)
{
	// 引数チェック
	if (g == NULL || p == NULL)
	{
		return false;
	}

	Zero(g, sizeof(WT_GATE_CONNECT_PARAM));

	if (PackGetStr(p, "Msid", g->Msid, sizeof(g->Msid)) == false)
	{
		Debug("PackGetStr(p, Msid, g->Msid, sizeof(g->Msid)) == false\n");
		return false;
	}

	g->Expires = PackGetInt64(p, "Expires");

	if (PackGetData2(p, "GateId", g->GateId, sizeof(g->GateId)) == false)
	{
		Debug("if (PackGetData2(p, GateId, g->GateId, sizeof(g->GateId)) == false)\n");
		return false;
	}

	if (PackGetData2(p, "Signature", g->Signature, sizeof(g->Signature)) == false)
	{
		Debug("if (PackGetData2(p, Signature, g->Signature, sizeof(g->Signature)) == false)\n");
		return false;
	}

	g->Cert = PackGetX(p, "Cert");
	if (g->Cert == NULL)
	{
		Debug("if (g->Cert == NULL)\n");
		return false;
	}

	return true;
}

// WT_GATE_CONNECT_PARAM を Pack に変換
void WtGateConnectParamToPack(PACK *p, WT_GATE_CONNECT_PARAM *g)
{
	// 引数チェック
	if (p == NULL || g == NULL)
	{
		return;
	}

	PackAddStr(p, "Msid", g->Msid);
	PackAddInt64(p, "Expires", g->Expires);
	PackAddData(p, "GateId", g->GateId, sizeof(g->GateId));
	PackAddData(p, "Signature", g->Signature, sizeof(g->Signature));
	PackAddX(p, "Cert", g->Cert);
}

// WT_GATE_CONNECT_PARAM の内容をバッファに変換
BUF *WtGateConnectParamPayloadToBuf(WT_GATE_CONNECT_PARAM *g)
{
	BUF *b;
	// 引数チェック
	if (g == NULL)
	{
		return NULL;
	}

	b = NewBuf();
	WriteBuf(b, g->Msid, StrLen(g->Msid));
	WriteBufInt64(b, g->Expires);
	WriteBuf(b, g->GateId, sizeof(g->GateId));

	return b;
}

// WT_GATE_CONNECT_PARAM の作成
WT_GATE_CONNECT_PARAM *WtNewGateConnectParam(char *msid, UINT64 expires, UCHAR *gateid, X *cert, UCHAR *signature)
{
	WT_GATE_CONNECT_PARAM *p;
	// 引数チェック
	if (msid == 0 || gateid == 0 || cert == 0 || signature == 0)
	{
		return NULL;
	}

	p = ZeroMalloc(sizeof(WT_GATE_CONNECT_PARAM));
	StrCpy(p->Msid, sizeof(p->Msid), msid);
	p->Expires = expires;
	Copy(p->GateId, gateid, SHA1_SIZE);
	p->Cert = CloneX(cert);
	Copy(p->Signature, signature, 128);

	return p;
}

// WT_GATE_CONNECT_PARAM のクローン
WT_GATE_CONNECT_PARAM *WtCloneGateConnectParam(WT_GATE_CONNECT_PARAM *p)
{
	WT_GATE_CONNECT_PARAM *ret;
	// 引数チェック
	if (p == NULL)
	{
		return NULL;
	}

	ret = ZeroMalloc(sizeof(WT_GATE_CONNECT_PARAM));
	Copy(ret, p, sizeof(WT_GATE_CONNECT_PARAM));
	ret->Cert = CloneX(p->Cert);

	return ret;
}

// WT_GATE_CONNECT_PARAM の解放
void WtFreeGateConnectParam(WT_GATE_CONNECT_PARAM *p)
{
	FreeX(p->Cert);
	Free(p);
}

// Gate のリスナースレッド
void WtgAcceptThread(THREAD *thread, void *param)
{
	TCP_ACCEPTED_PARAM *accepted_param;
	LISTENER *r;
	SOCK *s;
	WT *wt;
	// 引数チェック
	if (thread == NULL || param == NULL)
	{
		return;
	}

	accepted_param = (TCP_ACCEPTED_PARAM *)param;
	r = accepted_param->r;
	s = accepted_param->s;
	AddRef(r->ref);
	AddRef(s->ref);
	wt = (WT *)r->ThreadParam;
	AddRef(wt->Ref);

	AddSockThread(wt->SockThreadList, s, thread);

	NoticeThreadInit(thread);
	AcceptInit(s);

	WtgAccept(wt, s);

	DelSockThread(wt->SockThreadList, s);

	ReleaseSock(s);
	ReleaseListener(r);
	ReleaseWt(wt);
}

// Gate の開始
void WtgStart(WT *wt, X *cert, K *key, UINT port)
{
	// 引数チェック
	if (wt == NULL || cert == NULL || key == NULL || port == 0)
	{
		return;
	}

	// メモリサイズの節約
	SetFifoCurrentReallocMemSize(65536);

	wt->GateCert = CloneX(cert);
	wt->GateKey = CloneK(key);

	Rand(wt->GateId, sizeof(wt->GateId));

	wt->SessionList = NewList(WtgCompareSession);

	// ソケットとスレッドのリストの作成
	wt->SockThreadList = NewSockThreadList();

	// リスナーの開始
	wt->Port = port;
	wt->Listener = NewListenerEx(wt->Cedar, LISTENER_TCP, port, WtgAcceptThread, wt);
}

// TTCP の切断
void WtDisconnectTTcp(TTCP *ttcp)
{
	// 引数チェック
	if (ttcp == NULL)
	{
		return;
	}

	Disconnect(ttcp->Sock);
	ttcp->Disconnected = true;
}

// Gate の停止
void WtgStop(WT *wt)
{
	// 引数チェック
	if (wt == NULL)
	{
		return;
	}

	// リスナーの停止
	StopAllListener(wt->Cedar);
	StopListener(wt->Listener);
	ReleaseListener(wt->Listener);

	// 接続中のすべてのソケットと対応するスレッドの削除
	FreeSockThreadList(wt->SockThreadList);
	wt->SockThreadList = NULL;

	ReleaseList(wt->SessionList);

	// リソースの解放
	FreeX(wt->GateCert);
	FreeK(wt->GateKey);
}


