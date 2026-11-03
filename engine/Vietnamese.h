//
//  Vietnamese.h
//  OpenKey
//
//  Created by Tuyen on 1/19/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef Vietnamese_h
#define Vietnamese_h
#include "DataType.h"
#include <vector>
#include <map>

using namespace std;

extern map<uint16_t, vector<vector<uint16_t>>> vowelByKey;
extern map<uint16_t, vector<vector<uint32_t>>> vowelCombinations;
extern map<uint16_t, vector<vector<uint16_t>>> vowelsForMark;
extern vector<vector<uint16_t>> consonantsForD;
extern vector<vector<uint16_t>> consonantTable;
extern vector<vector<uint16_t>> endConsonantTable;
extern vector<uint16_t> standaloneWBad;
extern vector<vector<uint16_t>> doubleWAllowed;

extern map<uint32_t, vector<uint16_t>> unicodeTable;

extern map<uint32_t, uint32_t> characterMap;

extern uint16_t keyCodeToCharacter(const uint32_t& keyCode);
#endif /* Vietnamese_h */
