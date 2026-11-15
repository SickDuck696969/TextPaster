#pragma once
#include <cstdint>
#include <cstdlib>
#include <direct.h>
using u64=uint64_t;
#define mkdir(path,mode) _mkdir(path)
#define R_SUCCEEDED(rc) ((rc)==0)
inline int pmdmntGetProcessId(u64*,u64){return 1;}
constexpr u64 HidNpadButton_L=1ULL<<0;
constexpr u64 HidNpadButton_R=1ULL<<1;
constexpr u64 HidNpadButton_ZL=1ULL<<2;
constexpr u64 HidNpadButton_ZR=1ULL<<3;
constexpr u64 HidNpadButton_A=1ULL<<4;
constexpr u64 HidNpadButton_B=1ULL<<5;
constexpr u64 HidNpadButton_X=1ULL<<6;
constexpr u64 HidNpadButton_Y=1ULL<<7;
constexpr u64 HidNpadButton_Plus=1ULL<<8;
constexpr u64 HidNpadButton_Minus=1ULL<<9;
constexpr u64 HidNpadButton_Up=1ULL<<10;
constexpr u64 HidNpadButton_Down=1ULL<<11;
constexpr u64 HidNpadButton_Left=1ULL<<12;
constexpr u64 HidNpadButton_Right=1ULL<<13;
constexpr u64 HidNpadButton_StickL=1ULL<<14;
constexpr u64 HidNpadButton_StickR=1ULL<<15;
