/**
 * \file	server_net.c
 * \author	Kota.M
 *
 * サーバー側ネットワークモジュール
 */

#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>



#include "server.h"


/**
 * \defgroup	static functions
 */
static void send_data(int, void*, int);
static int  receive_data(int, void*, int);
// static char ReceiveCommand(int);						//			コマンド受信関数
static void HandleError(char*);                         //          エラーハンドリング

/**
 * グローバル変数
 */
static int num_socks;                                   //          ソケット数（FD最大番号+1）
static fd_set mask;                                     //          FD集合
static fd_set read_flag;								//			FD集合のコピー
static ClientInfo clients[player_num_init];             //          クライアント情報
// static int end_flg[player_num_init] = {0};



/** サーバーを立ち上げる
 * \param port  ポート番号
 */
void SetupServer(u_short port) {
	int rsock, sock = 0;                 //		接続要求受付用ソケット
	struct sockaddr_in sv_addr, cl_addr; //		ソケットアドレス関連の情報を格納

#ifdef DEBUG
	fprintf(stderr, "Server setup is started.\n");
#endif

	/* ソケット作成 */
	rsock = socket(AF_INET, SOCK_STREAM, 0);
	if (rsock < 0) {
		HandleError("socket()");
	}

#ifdef DEBUG
	fprintf(stderr, "sock() for request socket is done successfully.\n");
#endif

	/* ソケットの基本設定 */
	sv_addr.sin_family      = AF_INET;
	sv_addr.sin_port        = htons(port);
	sv_addr.sin_addr.s_addr = INADDR_ANY;

	int opt = 1;
	setsockopt(rsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	/* ソケットをアドレスと結びつける */
	if (bind(rsock, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) != 0) {
		HandleError("bind()");
	}

#ifdef DEBUG
	fprintf(stderr, "bind() is done successfully.\n");
#endif

	/* クライアントからの接続を待機 */
	if ( listen(rsock, player_num_init) != 0 ) {
		HandleError("listen()");
	}

#ifdef DEBUG
	fprintf(stderr, "listen() is started.\n");
#endif

	int i, max_sock = 0;
	socklen_t len;
	char src[MAX_L_ADDR];

	for ( i = 0; i < player_num_init; i++ ) {
		/* クライアントからの接続要求があるまで待機 */
		len  = sizeof(cl_addr);
		sock = accept(rsock, (struct sockaddr *)&cl_addr, &len);
		/* クライアントのソケット取得 */
		if (sock < 0) {
			HandleError("accept()");
		}
		/* FDの最大番号を更新 */
		if (max_sock < sock) {
			max_sock = sock;
		}
		/* クライアント名を読み込む */
		if ( read(sock, clients[i].name, MAX_L_NAME) == -1 ) {
			HandleError("read()");
		}
		/* クライアント情報を設定 */
		clients[i].id   = i;
		clients[i].sock = sock;
		clients[i].addr = cl_addr;
		memset(src, 0, sizeof(src));
		inet_ntop(AF_INET, (struct sockaddr *)&cl_addr.sin_addr, src, sizeof(src));
		fprintf(stderr, " playerID:%d %s is accepted (address=%s, port=%d).\n", i, clients[i].name, src, ntohs(cl_addr.sin_port));
		/* 受け付けたクライアントに送信 */
		int temp_player_num = player_num_init;
		if ( write(clients[i].sock, &temp_player_num, sizeof(int)) == -1 ) {    //      参加人数
			HandleError("write()");
		}
		if ( write(clients[i].sock, &i, sizeof(int)) == -1 ) {                  //      ID
			HandleError("write()");
		}
	}

#ifdef DEBUG
	fprintf(stderr, "All player joined.\n");
#endif

	/* ソケットを閉じる */
	close(rsock);

	/* クライアント情報を送信 */
	for ( i = 0; i < player_num_init; i++ ) {
		for ( int j = 0; j < player_num_init; j++ ) {
			if ( write(clients[i].sock,  &clients[j].id, sizeof(int)) == -1 ) {
				HandleError("write()");
			}
			if ( write(clients[i].sock, clients[j].name, MAX_L_NAME) == -1 ) {
				HandleError("write()");
			}
		}
	}

#ifdef DEBUG
	fprintf(stderr, "All player data are sent.\n");
#endif

	num_socks = max_sock + 1;
	FD_ZERO(&mask);
	FD_SET(0, &mask);
	for ( i = 0; i < player_num_init; i++ ) {
		FD_SET(clients[i].sock, &mask);
	}

#ifdef DEBUG
	fprintf(stderr, "Server setup is done.\n");
#endif
}


/* サーバーを終了させる */
void TerminateServer(void)
{
	for (int i = 0; i < player_num_init; i++) {
		close(clients[i].sock);
	}

#ifdef DEBUG
	fprintf(stderr, "All connections are closed.\n");
#endif

	exit(0);
}



/** 迷路情報を送信 */
int BroadcastMapData(void) {
	for ( int cid = 0; cid < player_num_init; cid++ ) {
		for ( int i = 0; i < MAP_HEIGHT; i++ ) {
			for ( int j = 0; j < MAP_WIDTH; j++ ) {
				if ( write(clients[cid].sock, &(gServer.map[i][j]), sizeof(int) ) == -1 ) {
					fprintf(stderr, "マップおくれなかった, ぴえん\n");
					return SDL_FALSE;
				}
			}
		}
	}
	return SDL_TRUE;
}

/** プレイヤーのデータを送信する */
void SendPlayerData() {
	if (gServer.game_stts == GS_Playing){
		Manege_Client();
	}
	
	
	CONTAINER	data;
	SDL_LockMutex(gServer.Mutex);
	for ( int i = 0; i < player_num_init; i++ ) {
		data.pinfo[i].point.x = sPlayer_info_all[i].point.x;
		data.pinfo[i].point.y = sPlayer_info_all[i].point.y;
		data.pinfo[i].point.z = sPlayer_info_all[i].point.z;
		data.pinfo[i].vec.x = sPlayer_info_all[i].vector.x;
		data.pinfo[i].vec.y = sPlayer_info_all[i].vector.y;
		data.pinfo[i].vec.z = sPlayer_info_all[i].vector.z;
		data.pinfo[i].flont_vec.x = sPlayer_info_all[i].flont_vec.x;
		data.pinfo[i].flont_vec.y = sPlayer_info_all[i].flont_vec.y;
		data.pinfo[i].flont_vec.z = sPlayer_info_all[i].flont_vec.z;
		data.pinfo[i].HP = sPlayer_info_all[i].HP;

		if (sPlayer_info_all[i].HP <= 0){
			sPlayer_info_all[i].see = SDL_FALSE;
		}
		
		data.pinfo[i].see = sPlayer_info_all[i].see;

	}
	SDL_UnlockMutex(gServer.Mutex);
	
	/* 送信 */
	data.command = PLAYER_COMMAND;
	data.game_stts = gServer.game_stts;

	for ( int i = 0; i < player_num_init; i++ ) {
		send_data(clients[i].sock, &data, sizeof(data));
		#ifdef DEBUG
		time_t currentTime;
    	time(&currentTime);
    	struct tm *localTime = localtime(&currentTime);
		fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> player to %d.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec, i);
		#endif
	}


}

/** 全プレイヤーのデータを受信する */
void ReceivePlayerData(int id, CONTAINER data) {
	int pid = data.ID;
	
	SDL_LockMutex(gServer.Mutex);
	sPlayer_info_all[pid].point.x = data.pinfo[pid].point.x;
	sPlayer_info_all[pid].point.y = data.pinfo[pid].point.y;
	sPlayer_info_all[pid].point.z = data.pinfo[pid].point.z;
	sPlayer_info_all[pid].vector.x = data.pinfo[pid].vec.x;
	sPlayer_info_all[pid].vector.y = data.pinfo[pid].vec.y;
	sPlayer_info_all[pid].vector.z = data.pinfo[pid].vec.z;
	sPlayer_info_all[pid].flont_vec.x = data.pinfo[pid].flont_vec.x;
	sPlayer_info_all[pid].flont_vec.y = data.pinfo[pid].flont_vec.y;
	sPlayer_info_all[pid].flont_vec.z = data.pinfo[pid].flont_vec.z;
	sPlayer_info_all[pid].HP = data.pinfo[pid].HP;
	sPlayer_info_all[pid].see = data.pinfo[pid].see;

	gServer.game_stts = data.game_stts;

	sPlayer_info_all[pid].init = 1;

	SDL_UnlockMutex(gServer.Mutex);

#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> player from %d. point=(%.1f,%.1f,%.1f)\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec , pid,
			data.pinfo[pid].point.x, data.pinfo[pid].point.y, data.pinfo[pid].point.z);
#endif


	if (gServer.game_stts == GS_Playing){
		Manege_Client();
	}
	

	SendPlayerData(1);
}

/** アイテムのデータを送信する */
void SendItemData() {
	/* 送信 */
	CONTAINER	data;
	SDL_LockMutex(gServer.Mutex);
	for ( int i = 0; i < player_num_init * item_num_init + 2; i++ ) {
		data.iinfo[i].kind= sitem_info_all[i].kind;
		data.iinfo[i].point.x = sitem_info_all[i].point.x;
		data.iinfo[i].point.y = sitem_info_all[i].point.y;
		data.iinfo[i].point.z = sitem_info_all[i].point.z;
		data.iinfo[i].vec.x   = sitem_info_all[i].vec.x;
		data.iinfo[i].vec.y   = sitem_info_all[i].vec.y;
		data.iinfo[i].vec.z   = sitem_info_all[i].vec.z;
		data.iinfo[i].speed   = sitem_info_all[i].speed;
		data.iinfo[i].target  = sitem_info_all[i].target;
		data.iinfo[i].see     = sitem_info_all[i].see;
	}
	
	
	/* その他のデータをセットする */
	data.command = ITEM_COMMAND;
	SDL_UnlockMutex(gServer.Mutex);
	
	for ( int i = 0; i < player_num_init; i++ ) {
		send_data(clients[i].sock, &data, sizeof(data));
		#ifdef DEBUG
		time_t currentTime;
    	time(&currentTime);
    	struct tm *localTime = localtime(&currentTime);
		fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> item to %d.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec, i);
		#endif
	}


}






/** 全アイテムのデータを受信する */
void ReceiveItemData(int id, CONTAINER data) {
	int pid = data.ID;
	SDL_LockMutex(gServer.Mutex);
	for ( int i = pid * item_num_init; i < (pid + 1) * item_num_init; i++ ) {
		sitem_info_all[i].kind    = data.iinfo[i].kind;
		sitem_info_all[i].point.x = data.iinfo[i].point.x;
		sitem_info_all[i].point.y = data.iinfo[i].point.y;
		sitem_info_all[i].point.z = data.iinfo[i].point.z;
		sitem_info_all[i].vec.x   = data.iinfo[i].vec.x;
		sitem_info_all[i].vec.y   = data.iinfo[i].vec.y;
		sitem_info_all[i].vec.z   = data.iinfo[i].vec.z;
		sitem_info_all[i].speed   = data.iinfo[i].speed;
		sitem_info_all[i].see     = data.iinfo[i].see;                         
	}
	SDL_UnlockMutex(gServer.Mutex);

	SendItemData();

#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> item from %d.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec, id);
#endif
}

/** 鍵のデータを受信する */
void ReceiveKeyData(CONTAINER data) {
	SDL_LockMutex(gServer.Mutex);
	
	sitem_info_all[player_num_init * item_num_init + data.index].kind = 4;
	sitem_info_all[player_num_init * item_num_init + data.index].target = data.ID;
	sitem_info_all[player_num_init * item_num_init + data.index].see = SDL_FALSE;
	if (data.index == 0){
		gServer.key_1_ID = data.ID;
	}
	if (data.index == 1){
		gServer.key_2_ID = data.ID;
	}
	
	if (gServer.game_stts == GS_Playing){
		Manege_Client();
	}
	SDL_UnlockMutex(gServer.Mutex);

	SendItemData();

#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> Key from %d.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec, data.ID);
#endif
}

/** 被弾したことを送信する */
void SendHitData(int id) {
	CONTAINER	data;
	SDL_LockMutex(gServer.Mutex);
	data.index= sPlayer_info_all[id].HP;
	SDL_UnlockMutex(gServer.Mutex);
	
	/* 送信 */
	data.command = HIT_COMMAND;
	data.game_stts = gServer.game_stts;

	send_data(clients[id].sock, &data, sizeof(data));
	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> player to %d.\n", 
		localTime->tm_mday, localTime->tm_mon + 1,
       	localTime->tm_year + 1900, localTime->tm_hour,
       	localTime->tm_min, localTime->tm_sec, id);
	#endif
	


}

/** 被弾のデータを受信する */
void ReceiveHitData(CONTAINER data) {
	SDL_LockMutex(gServer.Mutex);
	sPlayer_info_all[data.ID].HP -= data.index;
	SDL_UnlockMutex(gServer.Mutex);

	SendHitData(data.ID);

#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> Hit data get.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
#endif
}

/** ゴールしたことを送信する */
void SendGoalData() {
	CONTAINER	data;
	SDL_LockMutex(gServer.Mutex);
	data.ID    = gServer.finish_ID;
	data.index = gServer.finish_team_ID;

	data.command = GAME_FINISHED;
	data.game_stts = gServer.game_stts;
	SDL_UnlockMutex(gServer.Mutex);
	
	for (int i = 0; i < player_num_init; i++){
		send_data(clients[i].sock, &data, sizeof(data));
	}
	

	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> Finish.\n", 
		localTime->tm_mday, localTime->tm_mon + 1,
       	localTime->tm_year + 1900, localTime->tm_hour,
       	localTime->tm_min, localTime->tm_sec);
	#endif
	


}

/** ゴールを受信する */
void ReceiveGoalData(CONTAINER data) {
	SDL_LockMutex(gServer.Mutex);
	gServer.finish_ID      = data.ID;
	gServer.finish_team_ID = data.index;

	gServer.game_stts = GS_Clear;

	SDL_UnlockMutex(gServer.Mutex);


	SendGoalData();

#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> Finish get from %d.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec, data.ID);
#endif
}


/** データの送受信管理
 * \returns	1...続行, 0...終了
*/
int ControlRequests(int id) {

	int sock = clients[id].sock;
	FD_ZERO(&read_flag);
	FD_SET(sock, &read_flag);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 30;

	int ready = select(num_socks, &read_flag, NULL, NULL, &timeout);
	if ( ready == -1 ) {			//		エラー
		HandleError("select()");
	} else if ( ready == 0 ) {		//		タイムアウト
		return 1;
	}


	if ( FD_ISSET(sock, &read_flag) ) {

		CONTAINER data;
		memset(&data, 0, sizeof(data));
		receive_data(sock, &data, sizeof(CONTAINER));

		switch ( data.command ) {
		case PLAYER_COMMAND:
			ReceivePlayerData(id, data);
			break;
		case ITEM_COMMAND:
			ReceiveItemData(id, data);
			break;
		case GAME_COMMAND:			        //		ゲームステータス
			gServer.game_stts = data.game_stts;
			SendPlayerData();
			break;
		case KEY_COMMAND:
			ReceiveKeyData(data);
			break;
		case HIT_COMMAND:
			ReceiveHitData(data);
			break;
		case GAME_FINISHED:
			ReceiveGoalData(data);
			break;
		default:
			#ifdef DEBUG
			fprintf(stderr, "%c is not a valid command.\n", data.command);
			// exit(1);
			#endif
			break;
		}

	}

	while (gServer.game_stts == GS_Clear){
		break;
	}
	
	if (gServer.game_stts == GS_Clear){
		return 0;
	}
	return 1;
	
}


static void send_data(int sock, void *data, int size) {
  if ((data == NULL) || (size <= 0)) {
    fprintf(stderr, "send_data(): data is illeagal.\n");
    exit(1);
  }

  if (write(sock, data, size) == -1) {
	HandleError("write()");
  }
}

static int receive_data(int sock, void *data, int size) {
  if ((data == NULL) || (size <= 0)) {
    fprintf(stderr, "receive_data(): data is illeagal.\n");
    exit(1);
  }

  return(read(sock, data, size));
}


/** エラーハンドリング
 * \param message 表示するエラーメッセージ
 */
static void HandleError(char *message) {
	perror(message);
	fprintf(stderr, "%d\n", errno);
	exit(-1);
}

/* end of server_net.c */