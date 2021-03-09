/***************************
 ***   www.solontay.com  ***
 *** georgy@solontay.com ***
 ***************************/
#pragma once

#include <string>
#include <vector>

extern size_t OLR_FOND_Port[2];
extern std::string OLR_FOND_MultiAddr[2];
extern size_t OLS_FOND_Port[2];
extern std::string OLS_FOND_MultiAddr[2];
extern std::string OLS_FOND_IP[2];

/*
	Initialization example
	.....
   OLR_FOND_Port[0] = 16041;
   OLR_FOND_Port[1] = 17041;
   OLR_FOND_MultiAddr[0] = "239.195.1.41";
   OLR_FOND_MultiAddr[1] = "239.195.1.169";
   OLS_FOND_Port[0] = 16042;
   OLS_FOND_Port[1] = 17042;
   OLS_FOND_MultiAddr[0] = "239.195.1.42";
   OLS_FOND_MultiAddr[1] = "239.195.1.170";
   OLS_FOND_IP[0] = "91.203.253.227";
   OLS_FOND_IP[1] = "91.203.255.227";
   ......
*/

extern void (*_onEquityBidLineChange)(const size_t /*instr*/, const size_t /*line*/, const double /*price*/, const size_t /*qty*/);
extern void (*_onEquityAskLineChange)(const size_t /*instr*/, const size_t /*line*/, const double /*price*/, const size_t /*qty*/);
extern void (*_onEquityNewBidLine)(const size_t /*instr*/, const size_t /*line*/, const double /*price*/, const size_t /*qty*/);
extern void (*_onEquityNewAskLine)(const size_t /*instr*/, const size_t /*line*/, const double /*price*/, const size_t /*qty*/);
extern void (*_onEquityBidLineRemove)(const size_t /*instr*/, const size_t /*line*/, const double /*price*/);
extern void (*_onEquityAskLineRemove)(const size_t /*instr*/, const size_t /*line*/, const double /*price*/);
extern void (*_onEquityRecoveryStart)(const size_t /*instr*/);
extern void (*_onEquityRecoveryComplete)(const size_t /*instr*/);

extern void (*_onFatalError)();
extern void (*_onInfoMessage)(const std::string /*text*/); 

/*
	Initialization example
	.........
	void printMessage(const std::string str) 
	{
		std::cout << str << std::endl;
	}
	
	........
	
	_onInfoMessage = printMessage;

*/

void getEquityAsk(const size_t instr, const size_t line, double& price, size_t& qty);
void getEquityBid(const size_t instr, const size_t line, double& price, size_t& qty);

size_t getEquityBidQty(const size_t instr, const double price);
size_t getEquityAskQty(const size_t instr, const double price);
double getEquityAvgBidPrice(const size_t instr, const size_t qty);
double getEquityAvgAskPrice(const size_t instr, const size_t qty);
size_t getEquityBidDepth(const size_t instr);
size_t getEquityAskDepth(const size_t instr);
double getEquityBestBid(const size_t instr);
double getEquityBestAsk(const size_t instr);

bool astsInit(const size_t workers,	const size_t hole_pack, std::vector<std::string> equity_tickers, const bool wsa_startup);
/*
	workers - number of threads for quotes processing
	hole_pack - packs gap for recovery start
	wsa_startup - if true -> init winsock
	equity_tickers - 4 symbols ticker string - for example "LKOH"
*/
