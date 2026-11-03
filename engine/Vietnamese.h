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

extern map<uint16_t, vector<vector<uint16_t>>> _vowel;
extern map<uint16_t, vector<vector<uint32_t>>> _vowelCombine;
extern map<uint16_t, vector<vector<uint16_t>>> _vowelForMark;
extern vector<vector<uint16_t>> _consonantD;
extern vector<vector<uint16_t>> _consonantTable;
extern vector<vector<uint16_t>> _endConsonantTable;
extern vector<uint16_t> _standaloneWbad;
extern vector<vector<uint16_t>> _doubleWAllowed;

extern map<uint32_t, vector<uint16_t>> _unicodeTable;

extern map<uint32_t, uint32_t> _characterMap;

extern uint16_t keyCodeToCharacter(const uint32_t& keyCode);
#endif /* Vietnamese_h */
