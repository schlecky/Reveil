#ifndef __reveil_h__
#define __reveil_h__

#define MAX_MENU_OPTIONS 5

#define DCF_DIR   P1DIR
#define DCF_OUT   P1OUT
#define DCF_IN    P1IN
#define DCF_IE    P1IE
#define DCF_REN   P1REN
#define DCF_IES   P1IES
#define DCF_PON   BIT2
#define DCF_DAT   BIT3
#define DCF_POW_ON    DCF_OUT &= ~DCF_PON
#define DCF_POW_OFF   DCF_OUT |= DCF_PON

#define BTN_REN   P2REN
#define BTN_OUT   P2OUT
#define BTN_IES   P2IES
#define BTN_IE    P2IE
#define BTN_IFG   P2IFG
#define BTN_MENU  BIT0
#define BTN_HAUT  BIT5
#define BTN_BAS   BIT2

#define BL_DIR    P2DIR
#define BL_PIN    BIT4
#define BL_SEL    P2SEL
#define BL_FADE_MAX   1024
#define BL_FADE_STEP  1

#define LAMP_DIR  P2DIR
#define LAMP_PIN  BIT1
#define LAMP_OUT  P2OUT
#define LAMP_SEL  P2SEL
#define LAMP_FADE_MAX   1024
#define LAMP_FADE_STEP  1

#define INV_DIR P2DIR
#define INV_PIN BIT3
#define INV_SEL P2SEL
#define INV_ON  TA1CCTL0 = OUTMOD_4
#define INV_OFF TA1CCTL0 = OUTMOD_0

#define HP_PIN BIT6
#define HP_SEL P1SEL
#define HP_DIR P1DIR

#define POW_PIN BIT4
#define POW_IN  P1IN
#define POW_DIR P1DIR
#define POW_IES P1IES
#define POW_IE  P1IE
#define POW_IFG P1IFG

#define HPOff TA0CCTL1 = OUTMOD_0
#define HPOn TA0CCTL1 = OUTMOD_3

#define PARAMS_X  10
#define MENU_X  6

#define DELAI_SNOOZE 300;

const unsigned char nomJours[] = "LunMarMerJeuVenSamDim";

const unsigned char customChars[] = { 31, 31, 31, 31, 0, 0, 0, 0,       // caractere0 : demi-rectangle haut
                                      0, 0, 0, 0, 31, 31, 31, 31,       // caractere1 : demi-rectangle bas
                                      0, 14, 21, 23, 17, 14, 0, 0,      // caractere2 : horloge
                                      4, 14, 14, 4, 0, 0, 0, 0,         // caractere3 : point haut
                                      4, 14, 14, 14, 31, 0, 4, 0,       // caractere4 : cloche
                                      0, 14, 17, 4, 10, 0, 4, 0,        // caractere5 : DCF
                                      4, 21, 14, 27, 14, 21, 4, 0,      // caractere6 : Soleil   
                                      2,  3,  2, 14, 30, 12, 0, 0,      // caractere7 : note de musique       
                                    };

const unsigned char bignums3[] = {1, 0, 0, 1, 255, ' ', ' ', 255, 0, 1, 1, 0,               // 0
                                  ' ', ' ', 1, 255, ' ', 0, ' ', 255, ' ', ' ', ' ', 255,   // 1
                                  1, 0, 0, 1, ' ', 1, 1, 0, 255, 1, 1, 1,                   // 2
                                  0, 0, 0, 1, ' ', 0, 0, 1, 0, 1, 1, 0,                     // 3
                                  255, ' ', ' ', 255, 255, 1, 1, 255, ' ', ' ', ' ', 255,   // 4
                                  255, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,                     // 5
                                  1, 0, 0, 0, 255, 0, 0, 1, 0, 1, 1, 0,                     // 6
                                  0, 0, 0, 255, ' ', ' ', 255, ' ', ' ', 255, ' ', ' ',     // 7
                                  1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,                       // 8
                                  1, 0, 0, 1, 0, 1, 1, 255, 1, 1, 1, 0                      // 9
                                  };
                                  
#define DO    0 
#define DOd   1 
#define RE    2 
#define REd   3 
#define MI    4 
#define FA    5 
#define FAd   6
#define SOL   7
#define SOLd  8
#define LA    9 
#define LAd   10 
#define SI    11
#define P     12

//const int frequence[12] = {1911, 1804, 1703, 1607, 1517, 1432, 1351, 1276, 1203, 1136, 1072, 1012};
const int frequence[12] = {3822, 3608, 3406, 3214, 3034, 2864, 2702, 2552, 2406, 2272, 2144, 2024};

//macgyver
#define T 4

//#define NMUSIQUE 1
#include "musique.c"                                                         
         
#define FADE_COUNT 60           
const int pwm[FADE_COUNT] = {0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 8, 10, 11, 12, 14, 
                     16, 17, 20, 22, 25, 28, 32, 35, 40, 45, 50, 57, 64, 71, 80, 90, 101, 114, 
                     128, 143, 161, 181, 203, 228, 256, 287, 322, 362, 406, 456, 512, 574, 645, 
                     724, 812, 912, 1024};

unsigned char jourDansMois[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


#define strAlarme1    "\4Alarme1"
#define strAlarme2    "\4Alarme2"
#define strHorloge    "\2Horloge"
#define strReglage    "Reglages"
#define strSortie     " Sortie"
#define strSoleil     "\6Soleil"
#define strEclairage  " Eclairage"
#define strSon        "\7Son"

#define strEcranMin   "Ecran Min"
#define strEcranMax   "Ecran Max"
#define strLampeMin   "Lampe Min"
#define strLampeMax   "Lampe Max"

#define strHeure      "Heure"
#define strJour       "Jour"
#define strDate       "Date"
#define strAvanceDCF  "O2"

#define strVolMin     "Vol. Min"
#define strVolMax     "Vol. Max"
#define strSonnerie   "Sonnerie"

#define strTest       "Test"
#define strVide       "    "

#define MENU_SORTIE     0

// Pour le menu principal
#define MENU_ALARME1    1
#define MENU_ALARME2    2
#define MENU_SOLEIL     3
#define MENU_HEURE      4
#define MENU_ECLAIRAGE  5
#define MENU_SON        6

// Pour le menu d'éclairage
#define MENU_ECRAN_MIN  1
#define MENU_ECRAN_MAX  2

// Pour le menu de reglage de l'heure
#define MENU_HEURE_HEURE  1
#define MENU_HEURE_JOUR   2
#define MENU_HEURE_DATE   3
#define MENU_HEURE_AVANCE 4

// Pour le menu du son
#define MENU_SON_SONNERIE 1
#define MENU_SON_VOLMIN   2
#define MENU_SON_VOLMAX   3

const char* optsMenuPrincipal[] = {strSortie, strAlarme1, strAlarme2, strSoleil, strHorloge, strEclairage, strSon};
const char* optsMenuEclairage[] = {strSortie+1, strEcranMin, strEcranMax, strVide};
const char* optsMenuHeure[] = {strSortie+1, strHeure, strJour, strDate, strAvanceDCF};
const char* optsMenuSon[] = {strSortie+1, strSonnerie, strVolMin, strVolMax};

#define NMUSIQUES 4
const char* strMusiques[] = {"McGyver  ","P Martini","Bells    ","Deutschl."};
const unsigned char *musiques[NBMUSIQUES] = {&macgyver[0],&pink_martini[0],&bells[0], &deutschland[0]};

//char** strMenus;

enum Ecran {
  ECRAN_HEURE,
  ECRAN_MENU_PRINCIPAL,
  ECRAN_MENU_ECLAIRAGE,
  ECRAN_MENU_HEURE,
  ECRAN_MENU_SON
  };

typedef struct{
  int heures;
  int minutes;
  int secondes;} Heure;
  
typedef struct{
  Heure heure;
  int jourSem;
  int jour;
  int mois;
  int annee;} Date;

typedef struct{
  enum Ecran ecran;
  char* titre;
  int nbOptions;
  const char** strOptions;
  } Menu;

enum Config{
  ALARM_ON = 1,
  DCF_ON = 2,
  SOLEIL_ON = 4,
  LEVER_SOLEIL = 8,
  JOUE_MUSIQUE = 16,
  SNOOZE = 32,
  BL_FADE = 64,
  ALARM2_ON = 128,
  FLASH_LIGHT = 256,
  };

enum Bouton{
  AUCUN = 0,
  HAUT = 1,
  BAS = 2,
  MENU = 4
  };

enum Maj {
  MAJ_RIEN = 0,         // communs
  MAJ_CLEAR = 1,
  MAJ_INIT = 1024,
  
  MAJ_HEURE3 = 2,       // Ecran principal
  MAJ_DATE = 4,
  MAJ_DCF = 8,
  MAJ_ALARME = 16,
  MAJ_SOLEIL = 32,
  MAJ_DEBUG = 64,
  MAJ_SECONDES = 128,
  
  MAJ_FLECHE = 2,       // Menu
  MAJ_MENU   = 4,
  MAJ_CURS = 8,
  
  MAJ_REGL_ALARME = 16, // Menu  Principal
  MAJ_REGL_SOLEIL = 32,
  MAJ_REGL_ALARME2 = 64,
  
  MAJ_ECR_MIN = 16,     // Menu Eclairage
  MAJ_ECR_MAX = 32,
  
  MAJ_REGL_HEURE = 16,  // Menu Horloge
  MAJ_REGL_JOURSEM = 32,
  MAJ_REGL_DATE = 64,
  MAJ_REGL_AVANCEO2 = 128,
  
  MAJ_REGL_SONNERIE = 16,  // Menu son
  MAJ_REGL_VOLMIN   = 32,
  MAJ_REGL_VOLMAX = 64   
  };
  
void reglageHeure(Heure* h,enum Maj type);
#endif
