#ifndef _GRAPHICS_H
#define _GRAPHICS_H


#include "port/gfx.h"

#include "Texture.h"
#include "Mesh.h"
#include "SceneNode.h"

struct Graphics
{
    int resX;
    int resY;
    unsigned int depth;
    unsigned char field3_0xc;
    unsigned char field4_0xd;
    unsigned char field5_0xe;
    unsigned char field6_0xf;
    unsigned int screenResX;
    unsigned int screenResY;
    unsigned char field9_0x18;
    unsigned char field10_0x19;
    unsigned char field11_0x1a;
    unsigned char field12_0x1b;
    BOOL isFullscreen;
    unsigned char field14_0x20;
    unsigned char field15_0x21;
    unsigned char field16_0x22;
    unsigned char field17_0x23;
    unsigned char field18_0x24;
    unsigned char field19_0x25;
    unsigned char field20_0x26;
    unsigned char field21_0x27;
    unsigned char field22_0x28;
    unsigned char field23_0x29;
    unsigned char field24_0x2a;
    unsigned char field25_0x2b;
    unsigned char field26_0x2c;
    unsigned char field27_0x2d;
    unsigned char field28_0x2e;
    unsigned char field29_0x2f;
    unsigned char field30_0x30;
    unsigned char field31_0x31;
    unsigned char field32_0x32;
    unsigned char field33_0x33;
    unsigned char field34_0x34;
    unsigned char field35_0x35;
    unsigned char field36_0x36;
    unsigned char field37_0x37;
    unsigned char field38_0x38;
    unsigned char field39_0x39;
    unsigned char field40_0x3a;
    unsigned char field41_0x3b;
    unsigned char field42_0x3c;
    unsigned char field43_0x3d;
    unsigned char field44_0x3e;
    unsigned char field45_0x3f;
    unsigned char field46_0x40;
    unsigned char field47_0x41;
    unsigned char field48_0x42;
    unsigned char field49_0x43;
    unsigned char field50_0x44;
    unsigned char field51_0x45;
    unsigned char field52_0x46;
    unsigned char field53_0x47;
    unsigned char field54_0x48;
    unsigned char field55_0x49;
    unsigned char field56_0x4a;
    unsigned char field57_0x4b;
    unsigned char field58_0x4c;
    unsigned char field59_0x4d;
    unsigned char field60_0x4e;
    unsigned char field61_0x4f;
    unsigned char field62_0x50;
    unsigned char field63_0x51;
    unsigned char field64_0x52;
    unsigned char field65_0x53;
    unsigned char field66_0x54;
    unsigned char field67_0x55;
    unsigned char field68_0x56;
    unsigned char field69_0x57;
    unsigned char field70_0x58;
    unsigned char field71_0x59;
    unsigned char field72_0x5a;
    unsigned char field73_0x5b;
    unsigned char field74_0x5c;
    unsigned char field75_0x5d;
    unsigned char field76_0x5e;
    unsigned char field77_0x5f;
    unsigned char field78_0x60;
    unsigned char field79_0x61;
    unsigned char field80_0x62;
    unsigned char field81_0x63;
    unsigned char field82_0x64;
    unsigned char field83_0x65;
    unsigned char field84_0x66;
    unsigned char field85_0x67;
    unsigned char field86_0x68;
    unsigned char field87_0x69;
    unsigned char field88_0x6a;
    unsigned char field89_0x6b;
    unsigned char field90_0x6c;
    unsigned char field91_0x6d;
    unsigned char field92_0x6e;
    unsigned char field93_0x6f;
    unsigned char field94_0x70;
    unsigned char field95_0x71;
    unsigned char field96_0x72;
    unsigned char field97_0x73;
    unsigned char field98_0x74;
    unsigned char field99_0x75;
    unsigned char field100_0x76;
    unsigned char field101_0x77;
    unsigned char field102_0x78;
    unsigned char field103_0x79;
    unsigned char field104_0x7a;
    unsigned char field105_0x7b;
    unsigned char field106_0x7c;
    unsigned char field107_0x7d;
    unsigned char field108_0x7e;
    unsigned char field109_0x7f;
    unsigned char field110_0x80;
    unsigned char field111_0x81;
    unsigned char field112_0x82;
    unsigned char field113_0x83;
    unsigned char field114_0x84;
    unsigned char field115_0x85;
    unsigned char field116_0x86;
    unsigned char field117_0x87;
    unsigned char field118_0x88;
    unsigned char field119_0x89;
    unsigned char field120_0x8a;
    unsigned char field121_0x8b;
    unsigned char field122_0x8c;
    unsigned char field123_0x8d;
    unsigned char field124_0x8e;
    unsigned char field125_0x8f;
    unsigned char field126_0x90;
    unsigned char field127_0x91;
    unsigned char field128_0x92;
    unsigned char field129_0x93;
    unsigned char field130_0x94;
    unsigned char field131_0x95;
    unsigned char field132_0x96;
    unsigned char field133_0x97;
    unsigned char field134_0x98;
    unsigned char field135_0x99;
    unsigned char field136_0x9a;
    unsigned char field137_0x9b;
    unsigned char field138_0x9c;
    unsigned char field139_0x9d;
    unsigned char field140_0x9e;
    unsigned char field141_0x9f;
    unsigned char field142_0xa0;
    unsigned char field143_0xa1;
    unsigned char field144_0xa2;
    unsigned char field145_0xa3;
    unsigned char field146_0xa4;
    unsigned char field147_0xa5;
    unsigned char field148_0xa6;
    unsigned char field149_0xa7;
    unsigned char field150_0xa8;
    unsigned char field151_0xa9;
    unsigned char field152_0xaa;
    unsigned char field153_0xab;
    unsigned char field154_0xac;
    unsigned char field155_0xad;
    unsigned char field156_0xae;
    unsigned char field157_0xaf;
    unsigned char field158_0xb0;
    unsigned char field159_0xb1;
    unsigned char field160_0xb2;
    unsigned char field161_0xb3;
    unsigned char field162_0xb4;
    unsigned char field163_0xb5;
    unsigned char field164_0xb6;
    unsigned char field165_0xb7;
    unsigned char field166_0xb8;
    unsigned char field167_0xb9;
    unsigned char field168_0xba;
    unsigned char field169_0xbb;
    unsigned char field170_0xbc;
    unsigned char field171_0xbd;
    unsigned char field172_0xbe;
    unsigned char field173_0xbf;
    unsigned char field174_0xc0;
    unsigned char field175_0xc1;
    unsigned char field176_0xc2;
    unsigned char field177_0xc3;
    unsigned char field178_0xc4;
    unsigned char field179_0xc5;
    unsigned char field180_0xc6;
    unsigned char field181_0xc7;
    unsigned char field182_0xc8;
    unsigned char field183_0xc9;
    unsigned char field184_0xca;
    unsigned char field185_0xcb;
    unsigned char field186_0xcc;
    unsigned char field187_0xcd;
    unsigned char field188_0xce;
    unsigned char field189_0xcf;
    unsigned char field190_0xd0;
    unsigned char field191_0xd1;
    unsigned char field192_0xd2;
    unsigned char field193_0xd3;
    unsigned char field194_0xd4;
    unsigned char field195_0xd5;
    unsigned char field196_0xd6;
    unsigned char field197_0xd7;
    unsigned char field198_0xd8;
    unsigned char field199_0xd9;
    unsigned char field200_0xda;
    unsigned char field201_0xdb;
    unsigned char field202_0xdc;
    unsigned char field203_0xdd;
    unsigned char field204_0xde;
    unsigned char field205_0xdf;
    unsigned char field206_0xe0;
    unsigned char field207_0xe1;
    unsigned char field208_0xe2;
    unsigned char field209_0xe3;
    unsigned char field210_0xe4;
    unsigned char field211_0xe5;
    unsigned char field212_0xe6;
    unsigned char field213_0xe7;
    unsigned char field214_0xe8;
    unsigned char field215_0xe9;
    unsigned char field216_0xea;
    unsigned char field217_0xeb;
    unsigned char field218_0xec;
    unsigned char field219_0xed;
    unsigned char field220_0xee;
    unsigned char field221_0xef;
    unsigned char field222_0xf0;
    unsigned char field223_0xf1;
    unsigned char field224_0xf2;
    unsigned char field225_0xf3;
    unsigned char field226_0xf4;
    unsigned char field227_0xf5;
    unsigned char field228_0xf6;
    unsigned char field229_0xf7;
    unsigned char field230_0xf8;
    unsigned char field231_0xf9;
    unsigned char field232_0xfa;
    unsigned char field233_0xfb;
    unsigned char field234_0xfc;
    unsigned char field235_0xfd;
    unsigned char field236_0xfe;
    unsigned char field237_0xff;
    unsigned char field238_0x100;
    unsigned char field239_0x101;
    unsigned char field240_0x102;
    unsigned char field241_0x103;
    unsigned char field242_0x104;
    unsigned char field243_0x105;
    unsigned char field244_0x106;
    unsigned char field245_0x107;
    unsigned char field246_0x108;
    unsigned char field247_0x109;
    unsigned char field248_0x10a;
    unsigned char field249_0x10b;
    unsigned char field250_0x10c;
    unsigned char field251_0x10d;
    unsigned char field252_0x10e;
    unsigned char field253_0x10f;
    unsigned char field254_0x110;
    unsigned char field255_0x111;
    unsigned char field256_0x112;
    unsigned char field257_0x113;
    unsigned char field258_0x114;
    unsigned char field259_0x115;
    unsigned char field260_0x116;
    unsigned char field261_0x117;
    unsigned char field262_0x118;
    unsigned char field263_0x119;
    unsigned char field264_0x11a;
    unsigned char field265_0x11b;
    unsigned char field266_0x11c;
    unsigned char field267_0x11d;
    unsigned char field268_0x11e;
    unsigned char field269_0x11f;
    unsigned char field270_0x120;
    unsigned char field271_0x121;
    unsigned char field272_0x122;
    unsigned char field273_0x123;
    unsigned char field274_0x124;
    unsigned char field275_0x125;
    unsigned char field276_0x126;
    unsigned char field277_0x127;
    unsigned char field278_0x128;
    unsigned char field279_0x129;
    unsigned char field280_0x12a;
    unsigned char field281_0x12b;
    unsigned char field282_0x12c;
    unsigned char field283_0x12d;
    unsigned char field284_0x12e;
    unsigned char field285_0x12f;
    unsigned char field286_0x130;
    unsigned char field287_0x131;
    unsigned char field288_0x132;
    unsigned char field289_0x133;
    void *pPrimarySurface;          // PORT: unused (DirectDraw primary surface)
    unsigned char field291_0x138;
    unsigned char field292_0x139;
    unsigned char field293_0x13a;
    unsigned char field294_0x13b;
    DWORD field295_0x13c;
    DWORD field296_0x140;
    unsigned char field297_0x144;
    unsigned char field298_0x145;
    unsigned char field299_0x146;
    unsigned char field300_0x147;
    unsigned char field301_0x148;
    unsigned char field302_0x149;
    unsigned char field303_0x14a;
    unsigned char field304_0x14b;
    unsigned char field305_0x14c;
    unsigned char field306_0x14d;
    unsigned char field307_0x14e;
    unsigned char field308_0x14f;
    unsigned char field309_0x150;
    unsigned char field310_0x151;
    unsigned char field311_0x152;
    unsigned char field312_0x153;
    unsigned char field313_0x154;
    unsigned char field314_0x155;
    unsigned char field315_0x156;
    unsigned char field316_0x157;
    unsigned char field317_0x158;
    unsigned char field318_0x159;
    unsigned char field319_0x15a;
    unsigned char field320_0x15b;
    unsigned char field321_0x15c;
    unsigned char field322_0x15d;
    unsigned char field323_0x15e;
    unsigned char field324_0x15f;
    unsigned char field325_0x160;
    unsigned char field326_0x161;
    unsigned char field327_0x162;
    unsigned char field328_0x163;
    unsigned char field329_0x164;
    unsigned char field330_0x165;
    unsigned char field331_0x166;
    unsigned char field332_0x167;
    unsigned char field333_0x168;
    unsigned char field334_0x169;
    unsigned char field335_0x16a;
    unsigned char field336_0x16b;
    unsigned char field337_0x16c;
    unsigned char field338_0x16d;
    unsigned char field339_0x16e;
    unsigned char field340_0x16f;
    unsigned char field341_0x170;
    unsigned char field342_0x171;
    unsigned char field343_0x172;
    unsigned char field344_0x173;
    unsigned char field345_0x174;
    unsigned char field346_0x175;
    unsigned char field347_0x176;
    unsigned char field348_0x177;
    unsigned char field349_0x178;
    unsigned char field350_0x179;
    unsigned char field351_0x17a;
    unsigned char field352_0x17b;
    unsigned char field353_0x17c;
    unsigned char field354_0x17d;
    unsigned char field355_0x17e;
    unsigned char field356_0x17f;
    unsigned char field357_0x180;
    unsigned char field358_0x181;
    unsigned char field359_0x182;
    unsigned char field360_0x183;
    unsigned char field361_0x184;
    unsigned char field362_0x185;
    unsigned char field363_0x186;
    unsigned char field364_0x187;
    unsigned char field365_0x188;
    unsigned char field366_0x189;
    unsigned char field367_0x18a;
    unsigned char field368_0x18b;
    unsigned char field369_0x18c;
    unsigned char field370_0x18d;
    unsigned char field371_0x18e;
    unsigned char field372_0x18f;
    unsigned char field373_0x190;
    unsigned char field374_0x191;
    unsigned char field375_0x192;
    unsigned char field376_0x193;
    unsigned char field377_0x194;
    unsigned char field378_0x195;
    unsigned char field379_0x196;
    unsigned char field380_0x197;
    unsigned char field381_0x198;
    unsigned char field382_0x199;
    unsigned char field383_0x19a;
    unsigned char field384_0x19b;
    unsigned char field385_0x19c;
    unsigned char field386_0x19d;
    unsigned char field387_0x19e;
    unsigned char field388_0x19f;
    unsigned char field389_0x1a0;
    unsigned char field390_0x1a1;
    unsigned char field391_0x1a2;
    unsigned char field392_0x1a3;
    unsigned char field393_0x1a4;
    unsigned char field394_0x1a5;
    unsigned char field395_0x1a6;
    unsigned char field396_0x1a7;
    unsigned char field397_0x1a8;
    unsigned char field398_0x1a9;
    unsigned char field399_0x1aa;
    unsigned char field400_0x1ab;
    unsigned char field401_0x1ac;
    unsigned char field402_0x1ad;
    unsigned char field403_0x1ae;
    unsigned char field404_0x1af;
    unsigned char field405_0x1b0;
    unsigned char field406_0x1b1;
    unsigned char field407_0x1b2;
    unsigned char field408_0x1b3;
    unsigned char field409_0x1b4;
    unsigned char field410_0x1b5;
    unsigned char field411_0x1b6;
    unsigned char field412_0x1b7;
    unsigned char field413_0x1b8;
    unsigned char field414_0x1b9;
    unsigned char field415_0x1ba;
    unsigned char field416_0x1bb;
    unsigned char field417_0x1bc;
    unsigned char field418_0x1bd;
    unsigned char field419_0x1be;
    unsigned char field420_0x1bf;
    unsigned char field421_0x1c0;
    unsigned char field422_0x1c1;
    unsigned char field423_0x1c2;
    unsigned char field424_0x1c3;
    unsigned char field425_0x1c4;
    unsigned char field426_0x1c5;
    unsigned char field427_0x1c6;
    unsigned char field428_0x1c7;
    unsigned char field429_0x1c8;
    unsigned char field430_0x1c9;
    unsigned char field431_0x1ca;
    unsigned char field432_0x1cb;
    unsigned char field433_0x1cc;
    unsigned char field434_0x1cd;
    unsigned char field435_0x1ce;
    unsigned char field436_0x1cf;
    unsigned char field437_0x1d0;
    unsigned char field438_0x1d1;
    unsigned char field439_0x1d2;
    unsigned char field440_0x1d3;
    unsigned char field441_0x1d4;
    unsigned char field442_0x1d5;
    unsigned char field443_0x1d6;
    unsigned char field444_0x1d7;
    unsigned char field445_0x1d8;
    unsigned char field446_0x1d9;
    unsigned char field447_0x1da;
    unsigned char field448_0x1db;
    unsigned char field449_0x1dc;
    unsigned char field450_0x1dd;
    unsigned char field451_0x1de;
    unsigned char field452_0x1df;
    unsigned char field453_0x1e0;
    unsigned char field454_0x1e1;
    unsigned char field455_0x1e2;
    unsigned char field456_0x1e3;
    unsigned char field457_0x1e4;
    unsigned char field458_0x1e5;
    unsigned char field459_0x1e6;
    unsigned char field460_0x1e7;
    unsigned char field461_0x1e8;
    unsigned char field462_0x1e9;
    unsigned char field463_0x1ea;
    unsigned char field464_0x1eb;
    unsigned char field465_0x1ec;
    unsigned char field466_0x1ed;
    unsigned char field467_0x1ee;
    unsigned char field468_0x1ef;
    unsigned char field469_0x1f0;
    unsigned char field470_0x1f1;
    unsigned char field471_0x1f2;
    unsigned char field472_0x1f3;
    unsigned char field473_0x1f4;
    unsigned char field474_0x1f5;
    unsigned char field475_0x1f6;
    unsigned char field476_0x1f7;
    unsigned char field477_0x1f8;
    unsigned char field478_0x1f9;
    unsigned char field479_0x1fa;
    unsigned char field480_0x1fb;
    unsigned char field481_0x1fc;
    unsigned char field482_0x1fd;
    unsigned char field483_0x1fe;
    unsigned char field484_0x1ff;
    unsigned char field485_0x200;
    unsigned char field486_0x201;
    unsigned char field487_0x202;
    unsigned char field488_0x203;
    unsigned char field489_0x204;
    unsigned char field490_0x205;
    unsigned char field491_0x206;
    unsigned char field492_0x207;
    unsigned char field493_0x208;
    unsigned char field494_0x209;
    unsigned char field495_0x20a;
    unsigned char field496_0x20b;
    unsigned char field497_0x20c;
    unsigned char field498_0x20d;
    unsigned char field499_0x20e;
    unsigned char field500_0x20f;
    unsigned char field501_0x210;
    unsigned char field502_0x211;
    unsigned char field503_0x212;
    unsigned char field504_0x213;
    unsigned char field505_0x214;
    unsigned char field506_0x215;
    unsigned char field507_0x216;
    unsigned char field508_0x217;
    unsigned char field509_0x218;
    unsigned char field510_0x219;
    unsigned char field511_0x21a;
    unsigned char field512_0x21b;
    unsigned char field513_0x21c;
    unsigned char field514_0x21d;
    unsigned char field515_0x21e;
    unsigned char field516_0x21f;
    unsigned char field517_0x220;
    unsigned char field518_0x221;
    unsigned char field519_0x222;
    unsigned char field520_0x223;
    unsigned char field521_0x224;
    unsigned char field522_0x225;
    unsigned char field523_0x226;
    unsigned char field524_0x227;
    unsigned char field525_0x228;
    unsigned char field526_0x229;
    unsigned char field527_0x22a;
    unsigned char field528_0x22b;
    unsigned char field529_0x22c;
    unsigned char field530_0x22d;
    unsigned char field531_0x22e;
    unsigned char field532_0x22f;
    unsigned char field533_0x230;
    unsigned char field534_0x231;
    unsigned char field535_0x232;
    unsigned char field536_0x233;
    unsigned char field537_0x234;
    unsigned char field538_0x235;
    unsigned char field539_0x236;
    unsigned char field540_0x237;
    unsigned char field541_0x238;
    unsigned char field542_0x239;
    unsigned char field543_0x23a;
    unsigned char field544_0x23b;
    unsigned char field545_0x23c;
    unsigned char field546_0x23d;
    unsigned char field547_0x23e;
    unsigned char field548_0x23f;
    unsigned char field549_0x240;
    unsigned char field550_0x241;
    unsigned char field551_0x242;
    unsigned char field552_0x243;
    unsigned char field553_0x244;
    unsigned char field554_0x245;
    unsigned char field555_0x246;
    unsigned char field556_0x247;
    unsigned char field557_0x248;
    unsigned char field558_0x249;
    unsigned char field559_0x24a;
    unsigned char field560_0x24b;
    unsigned char field561_0x24c;
    unsigned char field562_0x24d;
    unsigned char field563_0x24e;
    unsigned char field564_0x24f;
    unsigned char field565_0x250;
    unsigned char field566_0x251;
    unsigned char field567_0x252;
    unsigned char field568_0x253;
    unsigned char field569_0x254;
    unsigned char field570_0x255;
    unsigned char field571_0x256;
    unsigned char field572_0x257;
    unsigned char field573_0x258;
    unsigned char field574_0x259;
    unsigned char field575_0x25a;
    unsigned char field576_0x25b;
    unsigned char field577_0x25c;
    unsigned char field578_0x25d;
    unsigned char field579_0x25e;
    unsigned char field580_0x25f;
    unsigned char field581_0x260;
    unsigned char field582_0x261;
    unsigned char field583_0x262;
    unsigned char field584_0x263;
    void *pBackBufferSurface;       // PORT: unused (DirectDraw back buffer)
    unsigned char field586_0x268;
    unsigned char field587_0x269;
    unsigned char field588_0x26a;
    unsigned char field589_0x26b;
    WORD field590_0x26c;
    WORD field591_0x26e;
    WORD field592_0x270;
    WORD field593_0x272;
    unsigned char field594_0x274;
    unsigned char field595_0x275;
    unsigned char field596_0x276;
    unsigned char field597_0x277;
    unsigned char field598_0x278;
    unsigned char field599_0x279;
    unsigned char field600_0x27a;
    unsigned char field601_0x27b;
    unsigned char field602_0x27c;
    unsigned char field603_0x27d;
    unsigned char field604_0x27e;
    unsigned char field605_0x27f;
    unsigned char field606_0x280;
    unsigned char field607_0x281;
    unsigned char field608_0x282;
    unsigned char field609_0x283;
    unsigned char field610_0x284;
    unsigned char field611_0x285;
    unsigned char field612_0x286;
    unsigned char field613_0x287;
    unsigned char field614_0x288;
    unsigned char field615_0x289;
    unsigned char field616_0x28a;
    unsigned char field617_0x28b;
    unsigned char field618_0x28c;
    unsigned char field619_0x28d;
    unsigned char field620_0x28e;
    unsigned char field621_0x28f;
    unsigned char field622_0x290;
    unsigned char field623_0x291;
    unsigned char field624_0x292;
    unsigned char field625_0x293;
    unsigned char field626_0x294;
    unsigned char field627_0x295;
    unsigned char field628_0x296;
    unsigned char field629_0x297;
    unsigned char field630_0x298;
    unsigned char field631_0x299;
    unsigned char field632_0x29a;
    unsigned char field633_0x29b;
    unsigned char field634_0x29c;
    unsigned char field635_0x29d;
    unsigned char field636_0x29e;
    unsigned char field637_0x29f;
    unsigned char field638_0x2a0;
    unsigned char field639_0x2a1;
    unsigned char field640_0x2a2;
    unsigned char field641_0x2a3;
    unsigned char field642_0x2a4;
    unsigned char field643_0x2a5;
    unsigned char field644_0x2a6;
    unsigned char field645_0x2a7;
    unsigned char field646_0x2a8;
    unsigned char field647_0x2a9;
    unsigned char field648_0x2aa;
    unsigned char field649_0x2ab;
    unsigned char field650_0x2ac;
    unsigned char field651_0x2ad;
    unsigned char field652_0x2ae;
    unsigned char field653_0x2af;
    unsigned char field654_0x2b0;
    unsigned char field655_0x2b1;
    unsigned char field656_0x2b2;
    unsigned char field657_0x2b3;
    unsigned char field658_0x2b4;
    unsigned char field659_0x2b5;
    unsigned char field660_0x2b6;
    unsigned char field661_0x2b7;
    unsigned char field662_0x2b8;
    unsigned char field663_0x2b9;
    unsigned char field664_0x2ba;
    unsigned char field665_0x2bb;
    unsigned char field666_0x2bc;
    unsigned char field667_0x2bd;
    unsigned char field668_0x2be;
    unsigned char field669_0x2bf;
    unsigned char field670_0x2c0;
    unsigned char field671_0x2c1;
    unsigned char field672_0x2c2;
    unsigned char field673_0x2c3;
    unsigned char field674_0x2c4;
    unsigned char field675_0x2c5;
    unsigned char field676_0x2c6;
    unsigned char field677_0x2c7;
    unsigned char field678_0x2c8;
    unsigned char field679_0x2c9;
    unsigned char field680_0x2ca;
    unsigned char field681_0x2cb;
    unsigned char field682_0x2cc;
    unsigned char field683_0x2cd;
    unsigned char field684_0x2ce;
    unsigned char field685_0x2cf;
    unsigned char field686_0x2d0;
    unsigned char field687_0x2d1;
    unsigned char field688_0x2d2;
    unsigned char field689_0x2d3;
    unsigned char field690_0x2d4;
    unsigned char field691_0x2d5;
    unsigned char field692_0x2d6;
    unsigned char field693_0x2d7;
    unsigned char field694_0x2d8;
    unsigned char field695_0x2d9;
    unsigned char field696_0x2da;
    unsigned char field697_0x2db;
    unsigned char field698_0x2dc;
    unsigned char field699_0x2dd;
    unsigned char field700_0x2de;
    unsigned char field701_0x2df;
    unsigned char field702_0x2e0;
    unsigned char field703_0x2e1;
    unsigned char field704_0x2e2;
    unsigned char field705_0x2e3;
    unsigned char field706_0x2e4;
    unsigned char field707_0x2e5;
    unsigned char field708_0x2e6;
    unsigned char field709_0x2e7;
    unsigned char field710_0x2e8;
    unsigned char field711_0x2e9;
    unsigned char field712_0x2ea;
    unsigned char field713_0x2eb;
    unsigned char field714_0x2ec;
    unsigned char field715_0x2ed;
    unsigned char field716_0x2ee;
    unsigned char field717_0x2ef;
    unsigned char field718_0x2f0;
    unsigned char field719_0x2f1;
    unsigned char field720_0x2f2;
    unsigned char field721_0x2f3;
    unsigned char field722_0x2f4;
    unsigned char field723_0x2f5;
    unsigned char field724_0x2f6;
    unsigned char field725_0x2f7;
    unsigned char field726_0x2f8;
    unsigned char field727_0x2f9;
    unsigned char field728_0x2fa;
    unsigned char field729_0x2fb;
    unsigned char field730_0x2fc;
    unsigned char field731_0x2fd;
    unsigned char field732_0x2fe;
    unsigned char field733_0x2ff;
    unsigned char field734_0x300;
    unsigned char field735_0x301;
    unsigned char field736_0x302;
    unsigned char field737_0x303;
    unsigned char field738_0x304;
    unsigned char field739_0x305;
    unsigned char field740_0x306;
    unsigned char field741_0x307;
    unsigned char field742_0x308;
    unsigned char field743_0x309;
    unsigned char field744_0x30a;
    unsigned char field745_0x30b;
    unsigned char field746_0x30c;
    unsigned char field747_0x30d;
    unsigned char field748_0x30e;
    unsigned char field749_0x30f;
    unsigned char field750_0x310;
    unsigned char field751_0x311;
    unsigned char field752_0x312;
    unsigned char field753_0x313;
    unsigned char field754_0x314;
    unsigned char field755_0x315;
    unsigned char field756_0x316;
    unsigned char field757_0x317;
    unsigned char field758_0x318;
    unsigned char field759_0x319;
    unsigned char field760_0x31a;
    unsigned char field761_0x31b;
    unsigned char field762_0x31c;
    unsigned char field763_0x31d;
    unsigned char field764_0x31e;
    unsigned char field765_0x31f;
    unsigned char field766_0x320;
    unsigned char field767_0x321;
    unsigned char field768_0x322;
    unsigned char field769_0x323;
    unsigned char field770_0x324;
    unsigned char field771_0x325;
    unsigned char field772_0x326;
    unsigned char field773_0x327;
    unsigned char field774_0x328;
    unsigned char field775_0x329;
    unsigned char field776_0x32a;
    unsigned char field777_0x32b;
    unsigned char field778_0x32c;
    unsigned char field779_0x32d;
    unsigned char field780_0x32e;
    unsigned char field781_0x32f;
    unsigned char field782_0x330;
    unsigned char field783_0x331;
    unsigned char field784_0x332;
    unsigned char field785_0x333;
    unsigned char field786_0x334;
    unsigned char field787_0x335;
    unsigned char field788_0x336;
    unsigned char field789_0x337;
    unsigned char field790_0x338;
    unsigned char field791_0x339;
    unsigned char field792_0x33a;
    unsigned char field793_0x33b;
    unsigned char field794_0x33c;
    unsigned char field795_0x33d;
    unsigned char field796_0x33e;
    unsigned char field797_0x33f;
    unsigned char field798_0x340;
    unsigned char field799_0x341;
    unsigned char field800_0x342;
    unsigned char field801_0x343;
    unsigned char field802_0x344;
    unsigned char field803_0x345;
    unsigned char field804_0x346;
    unsigned char field805_0x347;
    unsigned char field806_0x348;
    unsigned char field807_0x349;
    unsigned char field808_0x34a;
    unsigned char field809_0x34b;
    unsigned char field810_0x34c;
    unsigned char field811_0x34d;
    unsigned char field812_0x34e;
    unsigned char field813_0x34f;
    unsigned char field814_0x350;
    unsigned char field815_0x351;
    unsigned char field816_0x352;
    unsigned char field817_0x353;
    unsigned char field818_0x354;
    unsigned char field819_0x355;
    unsigned char field820_0x356;
    unsigned char field821_0x357;
    unsigned char field822_0x358;
    unsigned char field823_0x359;
    unsigned char field824_0x35a;
    unsigned char field825_0x35b;
    unsigned char field826_0x35c;
    unsigned char field827_0x35d;
    unsigned char field828_0x35e;
    unsigned char field829_0x35f;
    unsigned char field830_0x360;
    unsigned char field831_0x361;
    unsigned char field832_0x362;
    unsigned char field833_0x363;
    unsigned char field834_0x364;
    unsigned char field835_0x365;
    unsigned char field836_0x366;
    unsigned char field837_0x367;
    unsigned char field838_0x368;
    unsigned char field839_0x369;
    unsigned char field840_0x36a;
    unsigned char field841_0x36b;
    unsigned char field842_0x36c;
    unsigned char field843_0x36d;
    unsigned char field844_0x36e;
    unsigned char field845_0x36f;
    unsigned char field846_0x370;
    unsigned char field847_0x371;
    unsigned char field848_0x372;
    unsigned char field849_0x373;
    unsigned char field850_0x374;
    unsigned char field851_0x375;
    unsigned char field852_0x376;
    unsigned char field853_0x377;
    unsigned char field854_0x378;
    unsigned char field855_0x379;
    unsigned char field856_0x37a;
    unsigned char field857_0x37b;
    unsigned char field858_0x37c;
    unsigned char field859_0x37d;
    unsigned char field860_0x37e;
    unsigned char field861_0x37f;
    unsigned char field862_0x380;
    unsigned char field863_0x381;
    unsigned char field864_0x382;
    unsigned char field865_0x383;
    unsigned char field866_0x384;
    unsigned char field867_0x385;
    unsigned char field868_0x386;
    unsigned char field869_0x387;
    unsigned char field870_0x388;
    unsigned char field871_0x389;
    unsigned char field872_0x38a;
    unsigned char field873_0x38b;
    unsigned char field874_0x38c;
    unsigned char field875_0x38d;
    unsigned char field876_0x38e;
    unsigned char field877_0x38f;
    unsigned char field878_0x390;
    unsigned char field879_0x391;
    unsigned char field880_0x392;
    unsigned char field881_0x393;
    void *pSurface3;                // PORT: unused
    unsigned char field883_0x398;
    unsigned char field884_0x399;
    unsigned char field885_0x39a;
    unsigned char field886_0x39b;
    unsigned char field887_0x39c;
    unsigned char field888_0x39d;
    unsigned char field889_0x39e;
    unsigned char field890_0x39f;
    unsigned char field891_0x3a0;
    unsigned char field892_0x3a1;
    unsigned char field893_0x3a2;
    unsigned char field894_0x3a3;
    unsigned char field895_0x3a4;
    unsigned char field896_0x3a5;
    unsigned char field897_0x3a6;
    unsigned char field898_0x3a7;
    unsigned char field899_0x3a8;
    unsigned char field900_0x3a9;
    unsigned char field901_0x3aa;
    unsigned char field902_0x3ab;
    unsigned char field903_0x3ac;
    unsigned char field904_0x3ad;
    unsigned char field905_0x3ae;
    unsigned char field906_0x3af;
    unsigned char field907_0x3b0;
    unsigned char field908_0x3b1;
    unsigned char field909_0x3b2;
    unsigned char field910_0x3b3;
    void *pDD;                      // PORT: unused (DirectDraw)
    void *pDD7;                     // PORT: unused (DirectDraw 7)
    unsigned int field913_0x3bc;
    unsigned int field917_0x3c0;
    int baseDrawDistance; // 0x3c4, 16.16 route near limit before detail scaling
    int maxDrawDistance;  // 0x3c8, 16.16 route far limit

};

// Texture format description filled in by CGraphics::EnumTextureFormatsCallback
// PORT: the renderer's format and its layout instead of a DDSURFACEDESC2.
struct TextureFormat {
    int gfxFormat;              // GFX_FORMAT_*
    GfxPixelFormat format;
    BYTE bits[4];   // bits per channel: R, B, G, A (bump: dU, dV, L)
    BYTE shifts[4]; // shift of each channel mask
};

struct Unk0x006e0bb0 {
    DWORD field0x0;
    DWORD field0x4;
    DWORD field0x8;
    DWORD field0xc;
    DWORD flag200;
    DWORD flag100;
    DWORD flag1000;
    DWORD field0x1c;
    DWORD field0x20;
    DWORD field0x24;
    DWORD field0x28;
    DWORD field0x2c;
    DWORD field0x30;
    DWORD field0x34;
    DWORD field0x38;
    DWORD field0x3c;
    DWORD field0x40;
    DWORD field0x44;
    DWORD field0x48;
    DWORD field0x4c;
    DWORD field0x50;
    DWORD field0x54;
    DWORD field0x58;
    DWORD field0x5c;
    DWORD field0x60;
    DWORD field0x64;
    DWORD field0x68;
    DWORD field0x6c;
    DWORD field0x70;
    DWORD field0x74;
    DWORD field0x78;
    DWORD field0x7c;
    DWORD field0x80;
    DWORD field0x84;
    DWORD field0x88;
    DWORD field0x8c;
    DWORD field0x90;
    DWORD field0x94;
    DWORD field0x98;
    DWORD field0x9c;
    DWORD field0xa0;
    DWORD field0xa4;
    DWORD field0xa8;
    DWORD minTextureWidth;   // 0xac
    DWORD minTextureHeight;  // 0xb0
    DWORD maxTextureWidth;   // 0xb4
    DWORD maxTextureHeight;  // 0xb8
};

// PORT: the Direct3D objects are gone; vertex buffers are the renderer's.
struct D3DTextureManager {
    void *pDD;                                   // 0x0  PORT: unused (IDirect3D7)
    void *pD3D;                                  // 0x4  PORT: unused (IDirect3DDevice7)
    BYTE deviceGUID[16];                         // 0x8  PORT: unused
    GfxVertexBuffer* pVertexBuffers[100];          // 0x18
    int vertexBufferFill[100];                     // 0x1a8 vertices used in each buffer
    GfxVertexBuffer* pVertexBuffer1;               // 0x338
    GfxVertexBuffer* pVertexBuffer2;               // 0x33c
    GfxVertexBuffer* pVertexBuffer3;               // 0x340
    int field_0x344;                                // 0x344
    int field_0x348;                                // 0x348
    SceneNode *pRootNode;                           // 0x34c root of the scene graph
    TextureFormat* textureInfo1;         // 0x350 opaque RGB format
    TextureFormat* textureInfo2;         // 0x354 RGB format with alpha
    TextureFormat* textureInfo3;         // 0x358 DXT1
    TextureFormat* textureInfo4;         // 0x35c DXT5
    TextureFormat* textureInfo5;         // 0x360 bump map format
    BYTE ddpfZBuffer[32];                // 0x364 PORT: unused (z-buffer DDPIXELFORMAT)
    Texture* textureBuffer[2048];        // 0x384
    Texture* textureBuffer2[20];            // 0x2384
    BYTE field_0x23d4[0x10];             // 0x23d4
    int fixedProjection[16];             // 0x23e4 16.16 copy of the projection matrix
    FixMatrix viewMatrix;                // 0x2424 inverse of the camera world matrix
    int field_0x2464;                    // 0x2464
};

struct DDEnumDeviceBufferEntry
{
    union
    {
        struct
        {
            void* pGUID;        // +0x00 PORT: always NULL (primary device)
            BYTE  guid[16];     // +0x04
        } device;

        struct
        {
            DWORD caps[4];      // +0x00 PORT: unused (DDSCAPS2 scratch)
            DWORD    unknown18; // +0x10
        } caps;
    };

    DWORD capFlag80000;             // 0x14
    DWORD capRender16Bit;           // 0x18
    DWORD capFlag1;                 // 0x1C
    DWORD capFlag200;               // 0x20

    DWORD capTextureFilter1;        // 0x24
    DWORD capTextureFilter2;        // 0x28
    DWORD hasZBuffer;               // 0x2C
    DWORD zBufferBitDepth;          // 0x30
    DWORD capTextureFilter3;        // 0x34
    DWORD capHardwareRasterization; // 0x38

    DWORD unknown3C;                // 0x3C
};

struct DDDeviceEnumBuffer {
    DWORD count;
    DWORD reserved;
    DDEnumDeviceBufferEntry entries[10];
};

struct Entry {
    char unk_0x00[0x50];           // untraced — candidate: GUID, driver name, or other DDDEVICEIDENTIFIER fields
    char name[0x50];               // confirmed: device description, written via wsprintfA("%s", ...)
};

struct DisplayMode
{
    DWORD width;
    DWORD height;
    DWORD colourDepth;
};

struct Unk0x0065ff90 {
    BYTE guid[16];          // PORT: unused
    CHAR deviceDesc[0x50];
    CHAR deviceName[0x50];
    DWORD field_0xb0;
};

struct Unk0x00660040 {
    DWORD surfaceCap;
    BYTE padding[0xb0];
};

extern Graphics *g_pGraphics;

// GLOBAL: CMR2 0x00511598
// IID_IDirect3DRGBDevice

// GLOBAL: CMR2 0x005115a8
// IID_IDirect3DHALDevice

// GLOBAL: CMR2 0x005115c8
// IID_IDirect3DRefDevice

// GLOBAL: CMR2 0x005115e8
// IID_IDirect3DTnLHalDevice

// GLOBAL: CMR2 0x005114a8
// IID_IDirectDraw7

// GLOBAL: CMR2 0x00511578
// IID_IDirect3D7

class CGraphics {
public:
    static bool InitializeDirectX(void);
    static void SetDefaults(void);
    static void RecreateGraphicsDeviceAndResources(unsigned int screenWidth, unsigned int screenHeight, unsigned int colourDepth, unsigned int param4, unsigned int param5);
    static void ReleaseCachedTextureSurfaces(void);
    static BYTE EvictManagedTextureResources(void);
    static BOOL ReleaseDirect3D(void);
    static void ReleaseVertexBuffers(void);
    static int ReleaseSurfaces(void);
    static void SetSelectedDisplayDriverIndex(int param1);
    static void SetSelectedRenderDeviceIndex(int param1);
    static BOOL CreateSelectedDisplayAndRenderDevice(int screenWidth, int screenHeight, int colourDepth);
    static BOOL EnumerateAndProbeDisplayDevices(DDDeviceEnumBuffer* param1);
    static BOOL EnumerateDisplayDeviceGUIDCallback(DDDeviceEnumBuffer *pBuffer);
    static BOOL ProbeDisplayDriverCapabilities(DDEnumDeviceBufferEntry *device);
    static void EnumerateRenderDeviceCapabilities(DDEnumDeviceBufferEntry *device);
    static int CollectRenderDeviceCapabilitiesCallback(DDEnumDeviceBufferEntry *pEntry);
    static DWORD GetTextureFormatCap1(int param1);
    static INT32 GetSelectedDisplayDriverIndex(void);
    static DWORD GetTextureFormatCap80000(int param_1);
    static BOOL EnumerateDisplayDriverDescriptions(void);
    static HRESULT EnumerateCompatibleDisplayModes(int width, int height, int bpp);
    static int DeviceCanRender16Bit(int param1);
    static DWORD QueryAvailableVideoMemoryForCaps(DWORD caps);
    static BOOL IsDisplayModeEnumerated(int width, int height, int colourDepth);
    static void SelectMatchingDisplayMode(int width, int height, int colourDepth);
    static DWORD GetSelectedRenderDeviceSurfaceCaps(void);
    static HRESULT EnumeratePreferredHalDevices(void);
    static void BltTexture(Texture *pTexture, int surfaceIndex);
    static void UnlockTexture(Texture *pTexture);
    static unsigned int GetPixelRed(GfxLockedRect *pDesc, int x, int y);
    static unsigned int GetPixelAlpha(GfxLockedRect *pDesc, int x, int y);
    static BOOL FreeTextureBuffers(void);
    static unsigned int GetTextureMemoryKilobytes(void);
    static void SetPrimaryColourByte(BYTE param1);
    static void SetPrimaryColourBias(BYTE param1);
    static void SetSecondaryColourByte(BYTE param1);
    static void SetSecondaryColourBias(BYTE param1);
    static HRESULT CopyZBufferPixelFormat(void);
    static int GetDisplayDriverCount(void);
    static void GetDisplayDeviceNames(int index, LPSTR description, LPSTR name);
    static int GetSelectedRenderDeviceIndex(void);
    static int GetSelectedDisplayDeviceIx(void);
    static DWORD GetDisplayCount(void);
    static void GetDisplayMode(int index, DWORD *pWidth, DWORD *pHeight, DWORD *pColourDepth);
    static DWORD GetTextureFormatCap200(int param1);
    static void SetPrimaryColourScale(BYTE param1);
    static void SetSecondaryColourScale(BYTE param1);
    static TGAImageInfo *ParseTGAHeader(BYTE *pHeader);
    static void RestoreSurfaces(void);
    static int SetMipMapCount(int width, int height);
    static void GetMipMapSurfaces(Texture *pTexture);
    static int GetMipMapDataSize(Texture *pTexture);
    static int GetMipMapPixelCount(Texture *pTexture);
    static void BltMipMaps(Texture *pTexture);
    static void LockTexture(Texture *pTexture, RECT *pRect);
    static RenderTexture *CreateCubeMapSurfaces(RenderTexture *pTexture);

    // GLOBAL: CMR2 0x005210b4
    static DWORD m_cubeMapSize;

public:
    static void InvalidateBlendStateCache(void);
    static int GetSharedVertexMemoryKilobytes(void);
    static HRESULT EnumTextureFormatsCallback(int gfxFormat, GfxPixelFormat *pddpf);
    static void SelectTextureFormats(void);
    static BOOL CreateDirect3DDevice(int param1, int param2, int param3);
    static BOOL GetRasterCapabilityField84(void);
    static void AllocateSharedVertexBuffers(void);
    static void CacheRenderDeviceCapabilities(void);
    static void ConfigureDefaultRenderStates(BOOL param1, int param2);

    static void SetProjection(int fovX, int fovY, int farPlane, int nearPlane);
    static void GenerateBumpMap(Texture *pSrc, Texture *pDst);
    static void CreateTextureSurface(Texture *pTexture, int width, int height, unsigned int flags);
    static void RemapTextureAlpha(Texture *pTexture, WORD from0, WORD to0, WORD from1, WORD to1, WORD from2, WORD to2, int cacheSlot);
    static void BlendPixel(Texture *pTexture, unsigned int x, unsigned int y, BYTE *pColour);
    static Texture *LoadDDSTexture(DDSFile *pDDS, Texture *pTexture);
    static Texture *CreateTextureFromImageData(char *name, void *pData, unsigned int flags);
    static Texture *LoadDDSThenTGATexture(char *name, unsigned int flags);
    static Texture *LoadTGATexture(BYTE *pTGA, Texture *pTexture);
    static BYTE *SampleTGAPixel(unsigned int x, unsigned int y, TGAImageInfo *pInfo, unsigned int flags);
    static Texture *LoadTGABumpMap(BYTE *pTGA, Texture *pTexture);

    // GLOBAL: CMR2 0x00520b14
    static int m_unk0x00520b14;
    // GLOBAL: CMR2 0x00520b68
    static char m_strSuffixBU[4];
    // GLOBAL: CMR2 0x00520b6c
    static char m_strSuffixRU[4];
    // GLOBAL: CMR2 0x00520b70
    static char m_strSuffixBR[4];
    // GLOBAL: CMR2 0x0051c680
    static char m_strSuffixBODF[8];
    // GLOBAL: CMR2 0x0051c9e4
    static char m_strSuffixDIGIT[8];
    // GLOBAL: CMR2 0x0051c9ec
    static char m_strSuffixREVCT[8];
    static void SetClearColour(int unused, int r, int g, int b);
    static void ClearTarget(void);
    static BOOL ClearZBuffer(void);
    static void SetCullMode(int mode);
    static void SetZEnable(int enable);
    static void SetTextureAddressClamp(int clamp);
    static void SetZWriteEnable(int enable);
    static void SetTexCoordIndex(int stage, int index);

    // GLOBAL: CMR2 0x0059ce24
    static DWORD m_clearColour;
    // GLOBAL: CMR2 0x00597cb8
    static int m_cullMode;
    // GLOBAL: CMR2 0x0059ce34
    static int m_zEnable;
    // GLOBAL: CMR2 0x0059ce38
    static int m_textureAddressClamp;
    // GLOBAL: CMR2 0x0059ce3c
    static int m_zWriteEnable;
    // GLOBAL: CMR2 0x0059bd04
    static int m_texCoordIndex[8];

    // GLOBAL: CMR2 0x00511310
    static double m_oneOver65536;
    // GLOBAL: CMR2 0x005112e0
    static double m_65536;
    // GLOBAL: CMR2 0x00520b80
    static float m_projectionScale;
    // GLOBAL: CMR2 0x00520b84
    static float m_nearPlane;
    // GLOBAL: CMR2 0x00520b88
    static float m_farPlane;
    // GLOBAL: CMR2 0x00520b8c
    static float m_fovX;
    // GLOBAL: CMR2 0x00520b90
    static float m_fovY;
    // GLOBAL: CMR2 0x005207b0
    static int m_farPlaneFixed;
    // GLOBAL: CMR2 0x0065fb68
    static float m_projection11;
    // GLOBAL: CMR2 0x0065fa50
    static float m_projection22;
    // GLOBAL: CMR2 0x00660c00
    static float m_projection33;
    // GLOBAL: CMR2 0x0065fb64
    static float m_projection43;
    // GLOBAL: CMR2 0x006e0bb0
    static Unk0x006e0bb0 m_d3dDeviceDesc7;
    // GLOBAL: CMR2 0x00520b9c
    static char m_strSetDesktopTo16Bit[48];
    // GLOBAL: CMR2 0x0072d56c
    static int m_unk0x0072d56c;
    // GLOBAL: CMR2 0x00660bfc
    static BOOL m_unk0x00660bfc;

    // GLOBAL: CMR2 0x00660698
    static TextureFormat m_texFormat16;
    // GLOBAL: CMR2 0x0065fae0
    static TextureFormat m_texFormat16Alpha;
    // GLOBAL: CMR2 0x0065fc80
    static TextureFormat m_texFormatDXT1_16;
    // GLOBAL: CMR2 0x0065fbf8
    static TextureFormat m_texFormatDXT5_16;
    // GLOBAL: CMR2 0x0065fb70
    static TextureFormat m_texFormatBump16;
    // GLOBAL: CMR2 0x00660720
    static TextureFormat m_texFormat24;
    // GLOBAL: CMR2 0x0065fa58
    static TextureFormat m_texFormat32;
    // GLOBAL: CMR2 0x006607a8
    static TextureFormat m_texFormatDXT1_32;
    // GLOBAL: CMR2 0x00663450
    static TextureFormat m_texFormatDXT5_32;
    // GLOBAL: CMR2 0x00660c08
    static TextureFormat m_texFormatBump32;
    // GLOBAL: CMR2 0x00663b34
    static BOOL m_hasTexFormat16;
    // GLOBAL: CMR2 0x00663b38
    static BOOL m_hasTexFormatDXT1_16;
    // GLOBAL: CMR2 0x00663b3c
    static BOOL m_hasTexFormat16Alpha;
    // GLOBAL: CMR2 0x00663b40
    static BOOL m_hasTexFormatDXT5_16;
    // GLOBAL: CMR2 0x00663b44
    static BOOL m_hasTexFormat24;
    // GLOBAL: CMR2 0x00663b48
    static BOOL m_hasTexFormatDXT1_32;
    // GLOBAL: CMR2 0x00663b4c
    static BOOL m_hasTexFormat32;
    // GLOBAL: CMR2 0x00663b50
    static BOOL m_hasTexFormatDXT5_32;
    // GLOBAL: CMR2 0x00663b54
    static BOOL m_hasTexFormatBump16;
    // GLOBAL: CMR2 0x00663b58
    static BOOL m_hasTexFormatBump32;

private:
    friend void NetRace_PackCarState(struct Car *car);
    // GLOBAL: CMR2 0x0051615c
    static char m_strSettingConfigurationToDefault[36];

    // Mesh_Rebuild locks/unlocks a shared vertex buffer.
    friend void Mesh_Rebuild(Mesh *pMesh);
    friend int Graphics_CopyIntoFreeTextureSlot(void *pSource, int param2);
    friend void Graphics_CreateSharedWriteOnlyVertexBuffer(void);
    friend RenderTexture *Graphics_AllocateCubeMapTextureSlot(void);

    // GLOBAL: CMR2 0x00520b1c
    static int m_unk0x00520b1c;
    // GLOBAL: CMR2 0x00520b20
    static int m_unk0x00520b20;
    // GLOBAL: CMR2 0x00520b28
    static int m_unk0x00520b28;
    // GLOBAL: CMR2 0x0065fa24
    static unsigned int m_unk0x0065fa24;
    // GLOBAL: CMR2 0x0065fa38
    static Texture *m_unk0x0065fa38;     // texture set on the last changed stage

    static void SetCachedSourceDestinationBlend(int param1, int param2);
    static void ConfigureTextureStageBlendMode(int stage, Texture *pTexture);
    static void ApplyTextureStageChange(int stage, Texture *pTexture);
    // SceneNode_CreateRoot stores the root node in the texture manager.
    friend SceneNode *SceneNode_CreateRoot(void);
    friend void Graphics_SetTextureFactorAlpha(BYTE *pColour);
    friend void Graphics_ReloadTexture(Texture *pTexture);
    friend void Car_SwapWheelTextures(char mode, SceneNode **pWheels);
    friend void Graphics_SwitchAlphaBlendAndTest(int enable);
    friend void Graphics_PresentFrameAndResetCounters(void);
    friend void Graphics_MarkTexturesAndDrawMeshParts(Mesh *pMesh);
    friend void CarLight_CacheBodyTextures(int car, int unused1, int unused2, BYTE flag);
    friend void FixMatrix_ProjectWorldPointToView(int *pOut, FixVector *pPoint, SceneNode *pView);
    friend void Graphics_DrawMeshPartsByTexture(Mesh *pMesh);
    // Draws the mesh LOD record in use with the state its flags ask for.
    friend void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures);
    friend int Graphics_ReserveCubeMapsAndLoadEnvironment(char *name, int count, GenericFile *pFile, DWORD size);
    friend void Graphics_ProjectMeshVertexMidpoints(Mesh *pMesh);
    friend void Graphics_DrawCubeMappedShadowTriangles(Mesh *pMesh);
    // Mesh_DrawEnvMapped sets the texture transform of the projected cube map.
    friend void Mesh_DrawEnvMapped(Mesh *pMesh);
    friend void *Sector_RelocateStageMeshFile(BYTE *pData, struct SceneNode *parent, struct GenericFile *archive);
    friend void OptionPreview_UpdateWheelMeshVariants(int param1);
    friend Texture *Graphics_FindAndEnsureResidentTexture(char *name);
    friend void Events_Init(char *unused, int slot, char animate);
    friend void Scene_SetAmbient(BYTE *pColour, int boost);
    friend void Sprite_DrawLayer(int layer);
    friend void Quad2D_DrawLayer(unsigned int layer);
    friend void Graphics_SetLightingMode(int mode);
    friend void Scene_BeginShadowBatch(void);
    friend void Scene_EndShadowBatch(void);
    friend void Scene_SetLightAttenuation(SceneNode *pNode, int attenuation);
    friend void Scene_SetLightColour(SceneNode *pNode, int r, int g, int b);
    friend void Scene_DrawShadowBatches(unsigned int view);
    friend void Graphics_SetFog(int start, int end, int a, int b, DWORD colour);
    friend void Graphics_EnableFog(void);
    friend void Graphics_DisableFog(void);
    friend void Graphics_SetRenderTarget(Texture *pTexture);
    friend void StageObject_FreeAll(void);
    friend void Sector_RebuildNodeLists(void);
    friend void ScreenLine2D_Draw(int layer);
    friend void Line2D_Draw(void);
    friend void Tri2D_DrawLayer(int layer);
    friend void Scene_RestoreLights(void);
    friend void Scene_SetViewFromCamera(SceneNode *pCamera);
    friend void Scene_SetLightPosition(SceneNode *pNode, int x, int y, int z);
    friend void SceneNode_FlushTransforms(SceneNode *pNode);
    friend void Graphics_ReloadAllTextures(void);
    friend void Mesh_RefreshVertices(Mesh *pMesh);
    friend void Mesh_SetColourAndRefresh(Mesh *pMesh, DWORD colour);
    friend void Billboard_Draw(SceneNode *pCamera);
    friend SceneNode *Scene_CreateLight(int type, int r, int g, int b, FixVector *pPosition, FixAngles *pAngles, SceneNode *pParent);
    // Note: this friend shifts the register allocation of CreateSelectedDisplayAndRenderDevice (87.0% -> 85.8%).
    friend void Mesh_UploadVertices(Mesh *pMesh);
    friend void StageObject_FindPlayerRevTextures(int player);
    friend void Graphics_DrawMeshTextureBatches(Mesh *pMesh);
    friend int OptionMovie_Open(char *fileName, unsigned int trackIndex);
    friend void Game_DrawMeshTextureRuns(Mesh *pMesh);
    friend void StageTiming_RestoreDeviceStageResources(void);
    friend int OptionPreview_LoadStageGeometryRecord(int);

    // GLOBAL: CMR2 0x00520b78
    static D3DTextureManager* m_pTextureManager;

    // GLOBAL: CMR2 0x00520b7c
    static BOOL m_unk0x00520b7c;

    // GLOBAL: CMR2 0x00520be0
    static char m_direct3DHAL[13]; // "Direct3D HAL"

    // GLOBAL: CMR2 0x00520bcc
    static char m_direct3DTLHAL[18]; // "Direct3D T&L HAL"    

    // GLOBAL: CMR2 0x0065fa2c
    static unsigned int m_unk0x0065fa2c;
    // GLOBAL: CMR2 0x0065fa20
    static unsigned int m_textureCount;
    // GLOBAL: CMR2 0x0065fa3c
    static unsigned int m_lockedTextureCount;
    // GLOBAL: CMR2 0x0065aa88
    static LockedTexture m_lockedTextures[5];
    // Backup copies of textures (surfaces restored with BltTexture).
    // GLOBAL: CMR2 0x0065ae20
    static Texture m_textureCache[64];
    // GLOBAL: CMR2 0x00520b2c
    static unsigned int m_unk0x00520b2c;
    // GLOBAL: CMR2 0x00520b30
    static unsigned int m_unk0x00520b30;
    // GLOBAL: CMR2 0x0065fa44
    static int m_unk0x0065fa44;
    // GLOBAL: CMR2 0x0065fa48
    static int m_unk0x0065fa48;
public:
    // GLOBAL: CMR2 0x00511338
    static float m_oneOver128;
private:
    // GLOBAL: CMR2 0x00520b34
    static float m_unk0x00520b34;
    // GLOBAL: CMR2 0x00520b38
    static float m_unk0x00520b38;
    // GLOBAL: CMR2 0x0065aa80
    static BYTE m_tgaPixel[4];
    // GLOBAL: CMR2 0x00520b24
    static float m_bumpScale;
    // GLOBAL: CMR2 0x00520b3c
    static char m_ddsExtension[8];
    // GLOBAL: CMR2 0x00520b44
    static char m_tgaExtension[8];
    // GLOBAL: CMR2 0x0065ad08
    static TGAImageInfo m_tgaImageInfo;
    // GLOBAL: CMR2 0x00816a80
    static int m_unk0x00816a80;
    // GLOBAL: CMR2 0x00816a84
    static int m_unk0x00816a84;
    // GLOBAL: CMR2 0x00816ba8
    static int m_mipMapSurfaces[2];     // PORT: 1 when the mip level exists

    // GLOBAL: CMR2 0x0065fa28
    static unsigned int m_unk0x0065fa28;

    // GLOBAL: CMR2 0x006dd890
    static int m_unk0x006dd890;

    // GLOBAL: CMR2 0x006631f8
    static DisplayMode m_displays[10];

    // GLOBAL: CMR2 0x006634d8
    static Entry m_unk0x006634d8[10];

    // GLOBAL: CMR2 0x00663b18
    static int m_unk0x00663b18;
    
    // GLOBAL: CMR2 0x00663b1c
    static int m_unk0x00663b1c;

    // GLOBAL: CMR2 0x0065ff90
    static Unk0x0065ff90 m_unk0x0065ff90[10];
    
    // GLOBAL: CMR2 0x00663b20
    static int m_unk0x00663b20;
    
    // GLOBAL: CMR2 0x00663b24
    static int m_unk0x00663b24;

    // GLOBAL: CMR2 0x00663b2c
    static int m_selectedDisplayDeviceIx;

    // GLOBAL: CMR2 0x00663b28
    static DWORD m_displayCount; // maybe possibly

    // GLOBAL: CMR2 0x00663b30
    static int m_releaseSurfaceCallbackID;
    
    // GLOBAL: CMR2 0x0081709c
    static BOOL m_unk0x0081709c;

    // GLOBAL: CMR2 0x0065fd08
    static DDDeviceEnumBuffer m_unk0x0065fd08;

    // GLOBAL: CMR2 0x00816ce8
    static DDDeviceEnumBuffer m_displayDevicePool;

    // GLOBAL: CMR2 0x00817098
    static int m_lifetimeDisplayDeviceCount;

    // GLOBAL: CMR2 0x00817094
    static int m_totalPixelsForScreen;

    // GLOBAL: CMR2 0x00660040
    static Unk0x00660040 m_unk0x00660040[10];

    // Declared after the data members: a friend name added before them shifts
    // the register allocation of CreateSelectedDisplayAndRenderDevice (87.9% -> 86.8%).
    friend void Game_DrawWorldMeshNodesAnd2DLayer(void);
    friend void Game_DrawCulledSectorMeshes(void);
    friend void FarLayer_Draw(SceneNode *pCamera);
    friend void FarLayer_ApplyTexture(Texture *pTexture);
    friend void Game_DrawCulledSectorShadows(void);
    friend void Game_DrawViewMaskNodes(SceneNode *pNode, int bit);
    friend void Game_DrawViewMaskNode(SceneNode *pNode, int bit);
    friend void Game_DrawDeferredObjects(void);
    friend int Graphics_RenderNodeCubeMapFaces(SceneNode *pNode, int bit);
    friend int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
};


// PORT: the friends of CGraphics declared at namespace scope (MSVC 6 found
// them through the friend declarations alone).
void NetRace_PackCarState(struct Car *car);
void Mesh_Rebuild(Mesh *pMesh);
int Graphics_CopyIntoFreeTextureSlot(void *pSource, int param2);
void Graphics_CreateSharedWriteOnlyVertexBuffer(void);
RenderTexture *Graphics_AllocateCubeMapTextureSlot(void);
SceneNode *SceneNode_CreateRoot(void);
void Graphics_SetTextureFactorAlpha(BYTE *pColour);
void Graphics_ReloadTexture(Texture *pTexture);
void Car_SwapWheelTextures(char mode, SceneNode **pWheels);
void Graphics_SwitchAlphaBlendAndTest(int enable);
void Graphics_PresentFrameAndResetCounters(void);
void Graphics_MarkTexturesAndDrawMeshParts(Mesh *pMesh);
void StageObject_InitCarBodyDamageTextures(int car, int unused1, int unused2, BYTE flag);
void FixMatrix_ProjectWorldPointToView(int *pOut, FixVector *pPoint, SceneNode *pView);
void Graphics_DrawMeshPartsByTexture(Mesh *pMesh);
void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures);
int Graphics_ReserveCubeMapsAndLoadEnvironment(char *name, int count, GenericFile *pFile, DWORD size);
void Graphics_ProjectMeshVertexMidpoints(Mesh *pMesh);
void Graphics_DrawCubeMappedShadowTriangles(Mesh *pMesh);
void Mesh_DrawEnvMapped(Mesh *pMesh);
void *Sector_RelocateStageMeshFile(BYTE *pData, int param_2, unsigned int param_3);
void OptionPreview_UpdateWheelMeshVariants(int param1);
Texture *Graphics_FindAndEnsureResidentTexture(char *name);
void Events_Init(int unused, int slot, char animate);
void Scene_SetAmbient(BYTE *pColour, int boost);
void Sprite_DrawLayer(int layer);
void Quad2D_DrawLayer(unsigned int layer);
void Graphics_SetLightingMode(int mode);
void Scene_BeginShadowBatch(void);
void Scene_EndShadowBatch(void);
void Scene_SetLightAttenuation(SceneNode *pNode, int attenuation);
void Scene_SetLightColour(SceneNode *pNode, int r, int g, int b);
void Scene_DrawShadowBatches(unsigned int view);
void Graphics_SetFog(int start, int end, int a, int b, DWORD colour);
void Graphics_EnableFog(void);
void Graphics_DisableFog(void);
void Graphics_SetRenderTarget(Texture *pTexture);
void StageObject_FreeAll(void);
void Sector_RebuildNodeLists(void);
void ScreenLine2D_Draw(int layer);
void Line2D_Draw(void);
void Tri2D_DrawLayer(int layer);
void Scene_RestoreLights(void);
void Scene_SetViewFromCamera(SceneNode *pCamera);
void Scene_SetLightPosition(SceneNode *pNode, int x, int y, int z);
void SceneNode_FlushTransforms(SceneNode *pNode);
void Graphics_ReloadAllTextures(void);
void Mesh_RefreshVertices(Mesh *pMesh);
void Mesh_SetColourAndRefresh(Mesh *pMesh, DWORD colour);
void Billboard_Draw(SceneNode *pCamera);
SceneNode *Scene_CreateLight(int type, int r, int g, int b, FixVector *pPosition, FixAngles *pAngles, SceneNode *pParent);
void Mesh_UploadVertices(Mesh *pMesh);
void StageObject_FindPlayerRevTextures(int player);
void Graphics_DrawMeshTextureBatches(Mesh *pMesh);
int OptionMovie_Open(char *fileName, unsigned int trackIndex);
void Game_DrawMeshTextureRuns(Mesh *pMesh);
void StageTiming_RestoreDeviceStageResources(void);
int OptionPreview_LoadStageGeometryRecord(int);
void Game_DrawWorldMeshNodesAnd2DLayer(void);
void Game_DrawCulledSectorMeshes(void);
void Game_DrawCulledSectorShadows(void);
void Game_DrawViewMaskNodes(SceneNode *pNode, int bit);
void Game_DrawViewMaskNode(SceneNode *pNode, int bit);
void Game_DrawDeferredObjects(void);
int Graphics_RenderNodeCubeMapFaces(SceneNode *pNode, int bit);
int Game_DrawSceneViewport(int param1, int param2, void *param3, int bit, BYTE flag);

// A camera-facing sprite queued by Billboard_Add.
struct BillboardDef {
    FixVector pos;              // 0x0
    int top;                    // 0xc
    int left;                   // 0x10
    int bottom;                 // 0x14
    int right;                  // 0x18
    BYTE r, g, b, a;            // 0x1c
    short field_0x20;           // 0x20 rotation (12-bit angle)
    BYTE shade;                 // 0x22 0 = fully lit ... 256 = scene dark colour
    BYTE flags;                 // 0x23 1 mirrored, 2 lit by the scene light
    int field_0x24;
};
void Billboard_Add(BillboardDef *pDef, Texture *pTexture);

#endif
