#ifndef __reveil_h__
#define __reveil_h__

#define MAX_MENU_OPTIONS 5

#define DCF_OUT   P2OUT
#define DCF_IE    P2IE
#define DCF_REN   P2REN
#define DCF_IES   P2IES
#define DCF_PON   BIT0
#define DCF_DAT   BIT1

#define BTN_REN   P2REN
#define BTN_OUT   P2OUT
#define BTN_IES   P2IES
#define BTN_IE    P2IE
#define BTN_IFG   P2IFG
#define BTN_MENU  BIT3
#define BTN_HAUT  BIT4
#define BTN_BAS   BIT5

#define DELAI_INTER 10

#define BL_DIR    P2DIR
#define BL_PIN    BIT2
#define BL_SEL    P2SEL
#define BL_FADE_MAX   100
#define BL_FADE_STEP  5

#define PARAMS_X  11
#define MENU_X  6

unsigned char nomJours[] = "LunMarMerJeuVenSamDim";

unsigned char customChars[] = { 31, 31, 31, 31, 0, 0, 0, 0,       // caractere0 : demi-rectangle haut
                                0, 0, 0, 0, 31, 31, 31, 31,       // caractere1 : demi-rectangle bas
                                0, 14, 21, 23, 17, 14, 0, 0,      // caractere2 : horloge
                                4, 14, 14, 4, 0, 0, 0, 0,         // caractere3 : point haut
                                4, 14, 14, 14, 31, 0, 4, 0,       // caractere4 : cloche
                                0, 14, 17, 14, 10, 0, 4, 0,       // caractere5 : DCF
                                4, 21, 14, 27, 14, 21, 4, 0,      // caractere6 : Soleil                            
                              };

unsigned char bignums3[] = {
                            1, 0, 0, 1, 255, ' ', ' ', 255, 0, 1, 1, 0,               // 0
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

unsigned char jourDansMois[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


#define strAlarme     "\4Alarme"
#define strHeure      "\2Heure"
#define strReglage    "Reglages"
#define strSortie     " Sortie"
#define strSoleil     "\6Soleil"
#define strEclairage  " Eclairage"

#define strEcranMin   "Ecran Min"
#define strEcranMax   "Ecran Max"
#define strLampeMin   "Lampe Min"
#define strLampeMax   "Lampe Max"

#define strTest       "Test"

#define MENU_SORTIE     0

// Pour le menu principal
#define MENU_ALARME     1
#define MENU_SOLEIL     2
#define MENU_HEURE      3
#define MENU_ECLAIRAGE  4

// Pour le menu d'éclairage
#define MENU_ECRAN_MIN  1
#define MENU_ECRAN_MAX  2
#define MENU_SOLEIN_MIN 3
#define MENU_SOLEIL_MAX 4

char* optsMenuPrincipal[] = {strSortie, strAlarme, strSoleil, strHeure, strEclairage};
char* optsMenuEclairage[] = {strSortie+1, strEcranMin, strEcranMax, strLampeMin, strLampeMax};
char** strMenus;

enum Ecran {
  ECRAN_HEURE,
  ECRAN_MENU_PRINCIPAL,
  ECRAN_MENU_ECLAIRAGE
  };

typedef struct{
  int heures;
  int minutes;
  int secondes;} Heure;

typedef struct{
  enum Ecran ecran;
  char* titre;
  int nbOptions;
  char** strOptions;
  } Menu;

Heure heure, alarme;

enum Config{
  ALARM_ON = 1,
  DCF_ON = 2,
  SOLEIL_ON = 4
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
  
  MAJ_HEURE3 = 2,       // Ecran principal
  MAJ_DATE = 4,
  MAJ_DCF = 8,
  MAJ_ALARME = 16,
  MAJ_SOLEIL = 32,
  
  MAJ_FLECHE = 2,       // Menu
  MAJ_MENU   = 4,
  MAJ_CURS = 8,
  
  MAJ_REGL_HEURE = 16,   // Menu  Principal
  MAJ_REGL_ALARME = 32,
  MAJ_REGL_SOLEIL = 64,
  
  MAJ_ECR_MIN = 16,  // Menu Eclairage
  MAJ_ECR_MAX = 32,
  };
  
void reglageHeure(Heure* h,enum Maj type);
#endif
