/**
 * \file    client_net.c
 * \author  Kota.M
 *
 * クライアント側ネットワークモジュール
*/

#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "client.h"


/**
 * \defgroup    static functions
*/
static void send_data(void*, int);
static int  receive_data(void*, int);
// static char ReceiveCommand(void);			//			コマンド受信関数
static void HandleError(char*);                 //          エラーハンドリング


/**
 * グローバル変数
*/
static int numClients;                          //          クライアント数
static int myID;                                //          自身のID
static int sock;                                //          接続用ソケット
static int num_socks;                           //          FDの監視範囲(=FD最大番号+1)
static fd_set mask;                             //          FD集合
static fd_set read_flag;						//			FD集合のコピー
static ClientInfo clients[player_num_init];     //          クライアント情報


/** クライアントを立ち上げる
 * \param   serverName  サーバー名
 * \param   port        ポート番号
*/
void SetupClient(char *serverName, u_short port) {
	struct hostent *server;                 //      サーバーについての情報を格納
	struct sockaddr_in sv_addr;             //      （サーバーの）ソケットアドレス関連の情報を格納
	
	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] Trying to connect server %s (port = %d).\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec, serverName, port);
	#endif

	/* サーバへ接続 */
	if ( (server = gethostbyname(serverName)) == NULL ) {
		HandleError("gethostbyname()");
	}
	/* ソケット作成 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		HandleError("socket()");
	}
	/* ソケットの基本設定 */
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(port);
	sv_addr.sin_addr.s_addr = *(u_int*)server->h_addr_list[0];
	/* サーバへの通信リクエスト */
	if ( connect(sock, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) != 0 ) {
		HandleError("connect()");
	}

	#ifdef DEBUG
	fprintf(stderr, "Success to connect.\n");
	#endif

	/* クライアント情報をターミナルから設定 */
	// char usrName[MAX_L_NAME];
	// int len;
	// do {
	// 	char temp[256] = {0};
	// 	fprintf(stderr, "Input your name (max length:%d): ", MAX_L_NAME);
	// 	fgets(temp, sizeof(temp), stdin);
	// 	len = strlen(temp);
	// } while ( len <= 0 || MAX_L_NAME + 1 < len );
	// gGame.usrName[len - 1] = '\0';

	write(sock, gGame.usrName, strlen(gGame.usrName));                       		//      名前を送信

	/* 自身のクライアント情報を受け取る */
	read(sock, &numClients, sizeof(int));
	read(sock, &myID, sizeof(int));
	if ( myID < numClients - 1 ) {							//		まだ揃っていないならば
		fprintf(stderr, "Waiting for other player...\n");
	}
	gGame.player_ID = myID;
	player_info_all[gGame.player_ID].player_ID = myID;

	/* 全クライアントの情報を表示 */
	fprintf(stderr, "----------\n");
	for ( int i = 0; i < numClients; i++ ) {
		read(sock, &(clients[i].id), sizeof(int));
		read(sock, clients[i].name, MAX_L_NAME);
		fprintf(stderr, "ID %d: %s\n", clients[i].id, clients[i].name);
		memcpy(player_info_all[i].player_name, clients[i].name, strlen(gGame.usrName));
	}
	fprintf(stderr, "----------\n");

	num_socks = sock + 1;
	FD_ZERO(&mask);
	FD_SET(0, &mask);
	FD_SET(sock, &mask);

	read_flag = mask;

	#ifdef DEBUG
	fprintf(stderr, "Client setup is done.\n");
	#endif
}


/* クライアントを終了させる */
void TerminateClient(void) {
	/* ソケットを閉じる */
	char com = GAME_FINISHED;
	if ( write(sock, &com, sizeof(char)) == -1 ) {
		HandleError("write()");
	}
	close(sock);

#ifdef DEBUG
	fprintf(stderr, "Connection is closed.\n");
#endif
}

/** ゲームのステータスを送信 */
void SendGameStts(void) {
	/* データをセット */
	CONTAINER	data;
	data.command = GAME_COMMAND;
	data.ID = myID;
	data.game_stts = gGame.game_stts;

	/* 送信 */
	send_data(&data, sizeof(data));

	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> game_stts.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
}




/** 迷路情報を受信 */
int ReceiveMapData(void) {
	for ( int i = 0; i < MAP_HEIGHT; i++ ) {
		for ( int j = 0; j < MAP_WIDTH; j++ ) {
			if ( read(sock, &(gGame.map[i][j]), sizeof(int) ) < 0 ) {
				fprintf(stderr, "マップもらえなかった, ぴえん\n");
				return SDL_FALSE;
			}
		}
	}
	return SDL_TRUE;
	gGame.game_stts = GS_Playing;
}


/** プレイヤーのデータを送信する */
void SendPlayerData(void) {
	if (gGame.game_stts == GS_Playing){

	CONTAINER	data;

	SDL_LockMutex(gGame.Mutex);
	data.pinfo[gGame.player_ID].point.x = player_info_all[gGame.player_ID].point.x;
	data.pinfo[gGame.player_ID].point.y = player_info_all[gGame.player_ID].point.y;
	data.pinfo[gGame.player_ID].point.z = player_info_all[gGame.player_ID].point.z;
	data.pinfo[gGame.player_ID].vec.x = player_info_all[gGame.player_ID].vector.x;
	data.pinfo[gGame.player_ID].vec.y = player_info_all[gGame.player_ID].vector.y;
	data.pinfo[gGame.player_ID].vec.z = player_info_all[gGame.player_ID].vector.z;
	data.pinfo[gGame.player_ID].flont_vec.x = player_info_all[gGame.player_ID].flont_vec.x;
	data.pinfo[gGame.player_ID].flont_vec.y = player_info_all[gGame.player_ID].flont_vec.y;
	data.pinfo[gGame.player_ID].flont_vec.z = player_info_all[gGame.player_ID].flont_vec.z;
	data.pinfo[gGame.player_ID].HP = player_info_all[gGame.player_ID].HP;
	data.pinfo[gGame.player_ID].see = player_info_all[gGame.player_ID].see;

	data.ID = gGame.player_ID;
	data.game_stts = gGame.game_stts;

	SDL_UnlockMutex(gGame.Mutex);

	/* その他のデータをセットする */
	data.command = PLAYER_COMMAND;

	/* 送信する */
	send_data(&data, sizeof(data));

	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> player ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
	}
}

/** 全プレイヤーのデータを受信 */
void ReceivePlayerData(CONTAINER data) {
	SDL_LockMutex(gGame.Mutex);
	for ( int i = 0; i < player_num_init; i++ ) {
		if (i != gGame.player_ID) {
			player_info_all[i].point.x = data.pinfo[i].point.x;
			player_info_all[i].point.y = data.pinfo[i].point.y;
			player_info_all[i].point.z = data.pinfo[i].point.z;
			player_info_all[i].vector.x = data.pinfo[i].vec.x;
			player_info_all[i].vector.y = data.pinfo[i].vec.y;
			player_info_all[i].vector.z = data.pinfo[i].vec.z;
			player_info_all[i].flont_vec.x = data.pinfo[i].flont_vec.x;
			player_info_all[i].flont_vec.y = data.pinfo[i].flont_vec.y;
			player_info_all[i].flont_vec.z = data.pinfo[i].flont_vec.z;
			player_info_all[i].HP = data.pinfo[i].HP;
			player_info_all[i].see = data.pinfo[i].see;
		}

	}
	gGame.game_stts = data.game_stts;
	

	SDL_UnlockMutex(gGame.Mutex);

	#ifdef	DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> player ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
}

/** アイテムのデータを送信する */
void SendItemData(void) {
	if (gGame.game_stts == GS_Playing){
	/* アイテムを送信 */
	SDL_LockMutex(gGame.Mutex);
	CONTAINER	data;
	for ( int i = gGame.player_ID * item_num_init; i < (gGame.player_ID + 1) * item_num_init; i++ ) {
			data.iinfo[i].kind= item_info_all[i].kind;
			data.iinfo[i].point.x = item_info_all[i].point.x;
			data.iinfo[i].point.y = item_info_all[i].point.y;
			data.iinfo[i].point.z = item_info_all[i].point.z;
			data.iinfo[i].vec.x = item_info_all[i].vec.x;
			data.iinfo[i].vec.y = item_info_all[i].vec.y;
			data.iinfo[i].vec.z = item_info_all[i].vec.z;
			data.iinfo[i].speed = item_info_all[i].speed;
			data.iinfo[i].see = item_info_all[i].see;	
	
			
	}
		/* その他のデータをセットする */
		data.ID = gGame.player_ID;
		data.command = ITEM_COMMAND;
		
	SDL_UnlockMutex(gGame.Mutex);

	/* 送信する */
	send_data(&data, sizeof(data));
	
	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> item ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
	}
}

/** アイテムデータを受信する */
void ReceiveItemData(CONTAINER	data) {
	/* グローバルに格納 */
	SDL_LockMutex(gGame.Mutex);				//		相互排除
	for ( int i = 0; i < player_num_init * item_num_init + 2; i++ ) {
		if ((i < gGame.player_ID * item_num_init) || ((gGame.player_ID + 1) * item_num_init < i)){
			item_info_all[i].kind = data.iinfo[i].kind;
			item_info_all[i].point.x = data.iinfo[i].point.x;
			item_info_all[i].point.y = data.iinfo[i].point.y;
			item_info_all[i].point.z = data.iinfo[i].point.z;
			item_info_all[i].vec.x = data.iinfo[i].vec.x;
			item_info_all[i].vec.y = data.iinfo[i].vec.y;
			item_info_all[i].vec.z = data.iinfo[i].vec.z;
			item_info_all[i].speed = data.iinfo[i].speed;
			item_info_all[i].target = data.iinfo[i].target;
			item_info_all[i].see = data.iinfo[i].see;
		}
	}
	
	SDL_UnlockMutex(gGame.Mutex);

	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> item ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
}

void SendKeyData(int key_ID) {
	if (gGame.game_stts == GS_Playing){
		/* アイテムを送信 */
		SDL_LockMutex(gGame.Mutex);
		CONTAINER	data;			
		/* データをセットする */
		data.ID = gGame.player_ID;
		data.index = key_ID;
		data.command = KEY_COMMAND;

		SDL_UnlockMutex(gGame.Mutex);

		/* 送信する */
		send_data(&data, sizeof(data));

		#ifdef DEBUG
		time_t currentTime;
    	time(&currentTime);
    	struct tm *localTime = localtime(&currentTime);
		fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> key ok.\n", 
				localTime->tm_mday, localTime->tm_mon + 1,
    	       	localTime->tm_year + 1900, localTime->tm_hour,
    	       	localTime->tm_min, localTime->tm_sec);
		#endif
	}
}

void SendHitData(int ID, int HP) {
	if (gGame.game_stts == GS_Playing){
		/* アイテムを送信 */
		SDL_LockMutex(gGame.Mutex);
		CONTAINER	data;
		/* データをセットする */
		data.ID = ID;
		data.index = HP;
		data.command = HIT_COMMAND;

		SDL_UnlockMutex(gGame.Mutex);

		/* 送信する */
		send_data(&data, sizeof(data));

		#ifdef DEBUG
		time_t currentTime;
    	time(&currentTime);
    	struct tm *localTime = localtime(&currentTime);
		fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> Hit ok.\n", 
				localTime->tm_mday, localTime->tm_mon + 1,
    	       	localTime->tm_year + 1900, localTime->tm_hour,
    	       	localTime->tm_min, localTime->tm_sec);
		#endif
	}
}

/** 被弾データを受信する */
void ReceiveHitData(CONTAINER data) {
	/* グローバルに格納 */
	SDL_LockMutex(gGame.Mutex);				//		相互排除
	player_info_all[gGame.player_ID].HP = data.index;
	SDL_UnlockMutex(gGame.Mutex);

	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> Hit ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
}

void SendGoalData(void) {
	if (gGame.game_stts == GS_Playing){
	/* アイテムを送信 */
	SDL_LockMutex(gGame.Mutex);
	CONTAINER	data;			
	/* データをセットする */
	data.ID = gGame.player_ID;
	if (gGame.player_ID < player_num_init/2){
		data.index = 0;
	} else {
		data.index = 1;
	}
	
	data.command = GAME_FINISHED;
		
	SDL_UnlockMutex(gGame.Mutex);

	/* 送信する */
	send_data(&data, sizeof(data));
	
	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Send> key ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
	}
}
void ReceiveGoalData(CONTAINER data) {
	/* グローバルに格納 */
	SDL_LockMutex(gGame.Mutex);				//		相互排除
	gGame.finish_ID = data.ID;
	gGame.finish_team_ID = data.index;
	gGame.game_stts = data.game_stts;
	SDL_UnlockMutex(gGame.Mutex);

	printf("Team %d win (Player:%d)\n",gGame.finish_team_ID, gGame.finish_ID);

	#ifdef DEBUG
	time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
	fprintf(stderr, "[%02d/%02d/%04d:%02d:%02d:%02d] <Receive> Goal ok.\n", 
			localTime->tm_mday, localTime->tm_mon + 1,
           	localTime->tm_year + 1900, localTime->tm_hour,
           	localTime->tm_min, localTime->tm_sec);
	#endif
}


/** エラーハンドリング
 * \param message エラーメッセージ
*/
static void HandleError(char *message) {
	perror(message);
	fprintf(stderr, "%d\n", errno);
	exit(1);
}


int Network(void* data){
	#ifdef DEBUG
	fprintf(stderr, "network start\n");
	#endif

	/* メインループ */
	while ( gGame.game_stts != GS_Clear ) {

		fd_set read_flag = mask;

		struct timeval timeout;           //    タイムアウト時間を設定
		timeout.tv_sec = 0;
		timeout.tv_usec = 30;


		/* 読み込み可能状態を監視 */
		if ( select(num_socks, (fd_set *)&read_flag, NULL, NULL, &timeout) == -1 ) {		//		エラー
			HandleError("select()");
		} 
		
		if (gGame.network_player == 1){
			SendPlayerData();
			gGame.network_player = 0;
		}
		if (gGame.network_item == 1){
			SendItemData();
			gGame.network_item = 0;
		}
		
		if (FD_ISSET(sock, &read_flag)){
			//受信
			CONTAINER data;
			memset(&data, 0, sizeof(CONTAINER));
			receive_data(&data, sizeof(data));
			switch (data.command) {
			case PLAYER_COMMAND:
				ReceivePlayerData(data);
				break;
			case ITEM_COMMAND:
				ReceiveItemData(data);				//		アイテム情報
				break;
			case HIT_COMMAND:
				ReceiveHitData(data);
				break;
			case GAME_FINISHED:
				ReceiveGoalData(data);
				break;
			
			default:
				break;
			}
		}	
	}


	return 0;
}

static void send_data(void *data, int size) {
  if ((data == NULL) || (size <= 0)) {
    fprintf(stderr, "send_data(): data is illeagal.\n");
    exit(1);
  }

  if (write(sock, data, size) == -1) {
    HandleError("write()");
  }
}

static int receive_data(void *data, int size) {
  if ((data == NULL) || (size <= 0)) {
    fprintf(stderr, "receive_data(): data is illeagal.\n");
    exit(1);
  }

  return(read(sock, data, size));
}


/* end of client_net.c */