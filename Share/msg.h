#pragma once
#ifndef MSG_H
#define MSG_H


#define CLIENT_REQUEST 0
#define CLIENT_VERSUS 1
#define CLIENT_SETUP 2
#define CLIENT_PLAYER_MOVE 3
#define CLIENT_DISCONNECT 4
#define SERVER_MAIN_MENU 5
#define SERVER_APPROVED 6
#define SERVER_DENIED 7
#define SERVER_INVITE 8
#define SERVER_SETUP_REQUEST 9
#define SERVER_PLAYER_MOVE_REQUEST 10
#define SERVER_GAME_RESULTS 11
#define SERVER_WIN 12
#define SERVER_DRAW 13
#define SERVER_NO_OPPENNTS 14

#define CLIENT_REQUEST_LENG 14
//#define CLIENT_VERSUS 1
//#define CLIENT_SETUP 2
//#define CLIENT_PLAYER_MOVE 3
//#define CLIENT_DISCONNECT 4
//#define SERVER_MAIN_MENU 5
//#define SERVER_APPROVED 6
//#define SERVER_DENIED 7
#define SERVER_INVITE_LENG 14
//#define SERVER_SETUP_REQUEST 9
//#define SERVER_PLAYER_MOVE_REQUEST 10
//#define SERVER_GAME_RESULTS 11
//#define SERVER_WIN 12
//#define SERVER_DRAW 13
//#define SERVER_NO_OPPENNTS 14

#define SUCCESSFUL_CONNECT_MSG "Connected to server on "
#define FAILED__CONNECT_MSG "Failed connecting to server on "
#define WAITING_OPTIONS "Choose what to do next:\n1. Try to reconnect\n2. Exit\n"
#define SERVER_DENIED_REQ_1 "Server on "
#define SERVER_DENIED_REQ_2 "denied the connection request.\n"
#define SERVER_MAIN_MENU_MSG "Choose what to do next:\n1. Play against another client\n2. Quit\n"
#define SERVER_INVITE_MSG "Game is on!\n"
#define SERVER_SETUP_REQUEST_MSG "Choose your 4 digits:"
#define SERVER_PLAYER_MOVE_REQUEST_MSG "Choose your guess:"
#define GAME_RESULTS_MSG1 "Bulls: "
#define GAME_RESULTS_MSG2 "Cows: "
#define GAME_RESULTS_MSG3 " played: "
#define SERVER_WIN_MSG " won!\nopponents number was "
#define SERVER_DRAW_MSG "It's a tie\n"
#define SERVER_OPPONENT_QUIT_MSG "Opponent quit.\n"








#endif // MSG_H