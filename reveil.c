#include <legacymsp430.h>
#include <msp430g2553.h>

#include "reveil.h"
#include "hd77480.h"

Menu mPrincipal, mEclairage, mHeure, mSon;
Menu* menu;   // pointeur vers le menu courant
Heure alarme, alarme2, debutAlarme;
Date date;
int deltaX;
int delaiSoleil; // temps de lever du soleil en minutes
int avanceO2;    // avance en minutes spécifique 02
int WDTcnt, DCFCnt, DCFBit, soleilTimer, soleilValue, blTimer, blFadeValue, blFadeStep;
Date DCFDate;    // La Date actuelle (contient l'heure)
int debug;
int DCFParite, DCFErreur;
int timerSnooze, volMax, volMin, numSonnerie;
int numAlarme=0;
int blMax, blMin;
int flashCnt;
int cursOn, cursX, cursY;
int select, oldSelect, shiftMenu;
int iMusique, musiqueCnt, volume, nMusique;
const unsigned char *musique;
enum Maj aMAJ;
enum Ecran ecran;
enum Config config;
enum Bouton btnAppuye;  // Le bouton sur lequel on appuie

NAKED(_reset_vector__)
{
	/* place your startup code here */

	/* Make sure, the branch to main (or to your start
	   routine) is the last line in the function */
	__asm__ __volatile__("br #main"::);
}

void LCDWriteHeure(Heure* h)
{
  LCDWriteInt2(h->heures);
  LCDSend(':',SEND_CHR);
  LCDWriteInt2(h->minutes);
}

void LCDWriteJourSem()
{
  int i;
  for(i=0;i<3;i++)
    LCDSend(nomJours[3*date.jourSem+i],SEND_CHR);  
}

void LCDWriteDate(int wrtJourSem)
{ 
  if(wrtJourSem)
  {
    LCDWriteJourSem(); 
    LCDSend(' ',SEND_CHR);
  }
  LCDWriteInt2(date.jour);
  LCDSend('/',SEND_CHR);
  LCDWriteInt2(date.mois);
  LCDSend('/',SEND_CHR);
  LCDWriteInt4(date.annee);
}

void LCDWriteString(const unsigned char* str)
{
  int i;
  for(i=0; str[i]!=0; i++)
  {
    LCDSend(str[i],SEND_CHR);
  }
}

void LCDWriteHeure3()
{
  LCDSend(LCD_RETURNHOME,SEND_CMD);
 
  LCDSendBigNum3(date.heure.heures/10,0);
  LCDSendBigNum3(date.heure.heures%10,5);
  LCDSendBigNum3(date.heure.minutes/10,10);
  LCDSendBigNum3(date.heure.minutes%10,15);

  //double points
  LCDGotoXY(9,1);
  LCDSend(3,SEND_CHR);
  LCDGotoXY(9,2);
  LCDSend(3,SEND_CHR);
}

// definie l'intensité de l'éclairage de l'écran, bl est entre 0 et BL_FADE_MAX
void setBL(int bl)
{
  TA1CCR2 = BL_FADE_MAX-pwm[bl];
}

// definie l'intensité de l'éclairage du soleil, intens est entre 0 et LAMP_FADE_MAX
void setLamp(int intens)
{
  TA1CCR1 = LAMP_FADE_MAX-pwm[intens];
}

// define la frequence du haut parleur
void setFreqHP(int note,int octave, int vol)
{
  if(note==P)
  {
    TA0CCR1 = TA0CCR0;
    return;
  }
  else
  {
    TA0CCR0 = (frequence[note]>>octave)<<1;
    TA0CCR1 = TA0CCR0 - vol*2;
  }
}

inline void sonneAlarme()
{
  config |= JOUE_MUSIQUE; 
  musique = musiques[numSonnerie];
  nMusique = musique[0]*3+1;
  iMusique = 1;
  volume = volMin;
  musiqueCnt = 2;
}

inline void stopMusique()
{
  config &= ~JOUE_MUSIQUE;
  HPOff;
}

void beep(int vol)
{
  setFreqHP(1,1,vol);
  HPOn;
  Delay(10000);
  HPOff;
}

// change l'etat de l'indicateur DCF
void blinkDCF()
{
  config ^= DCF_ON;
  aMAJ|=MAJ_DCF;
}

// Met a jour le symbole d'alarme
void dispAlarme()
{
  // alarme numero 1
  LCDGotoXY(19,1);
  if(config & ALARM_ON)
    LCDSend(4,SEND_CHR);
  else
    LCDSend(' ',SEND_CHR);
    
  // alarme numero 2
  LCDGotoXY(19,2);
  if(config & ALARM2_ON)
    LCDSend(4,SEND_CHR);
  else
    LCDSend(' ',SEND_CHR);
}

// Met a jour le symbole du soleil
void dispSoleil()
{
  LCDGotoXY(19,3);
  if(config & SOLEIL_ON)
    LCDSend(6,SEND_CHR);
  else
    LCDSend(' ',SEND_CHR);
}

// procedure de mise à jour de l'affichage
void affichageHeure()
{
  if(aMAJ == MAJ_RIEN)
    return;    
  if(aMAJ & MAJ_INIT)
    LCDInit();     
  if(aMAJ & MAJ_CLEAR)
    LCDClear();     
  
  if(aMAJ & MAJ_SECONDES)
  {
    LCDGotoXY(16,3);
    LCDWriteInt2(date.heure.secondes);
    aMAJ &= ~ MAJ_DEBUG;
  }
  
  if(aMAJ & MAJ_DEBUG)
  {
    LCDGotoXY(16,3);
    LCDWriteInt2(debug);
  }
  
  if(aMAJ & MAJ_HEURE3)
    LCDWriteHeure3();
    
  if(aMAJ & MAJ_DCF)
  {
    LCDGotoXY(19,0);
    if(config & DCF_ON)
      LCDSend(' ',SEND_CHR);
    else
      LCDSend(5,SEND_CHR);
  }
  
  if(aMAJ & MAJ_ALARME)
    dispAlarme();
    
  if(aMAJ & MAJ_SOLEIL)
    dispSoleil();
    

  if(aMAJ & MAJ_DATE)
  {
    LCDGotoXY(0,3); 
    LCDWriteDate(1);
  }
  aMAJ = MAJ_RIEN;
}

void affichageMenu()
{
  if(aMAJ == MAJ_RIEN)
    return;
  
  if(aMAJ & MAJ_CLEAR)
    LCDClear(); 
  
  if(aMAJ & MAJ_MENU)
  {
    LCDClear();
    switch(menu->ecran){
      case ECRAN_MENU_PRINCIPAL:
        aMAJ |= MAJ_REGL_ALARME | MAJ_REGL_ALARME2 | MAJ_REGL_SOLEIL | MAJ_FLECHE ;
        break;
      case ECRAN_MENU_ECLAIRAGE:
        aMAJ |= MAJ_ECR_MIN | MAJ_ECR_MAX | MAJ_FLECHE;
        break;
      case ECRAN_MENU_HEURE:
        aMAJ |= MAJ_REGL_HEURE | MAJ_REGL_DATE | MAJ_REGL_JOURSEM | MAJ_REGL_AVANCEO2 | MAJ_FLECHE ;
      case ECRAN_MENU_SON:
        aMAJ |= MAJ_REGL_SONNERIE | MAJ_REGL_VOLMIN | MAJ_REGL_VOLMAX | MAJ_FLECHE ;
    }
    LCDGotoXY(MENU_X,0);
    LCDWriteString(menu->titre);
    
    // Affiche les textes des menus
    int iMenu;
    for(iMenu=0;iMenu<3;iMenu++)
    {
      LCDGotoXY(1,1);
      LCDWriteString(menu->strOptions[shiftMenu]);
      
      LCDGotoXY(1,2);
      LCDWriteString(menu->strOptions[(shiftMenu+1)%menu->nbOptions]);
      
      LCDGotoXY(1,3);
      LCDWriteString(menu->strOptions[(shiftMenu+2)%menu->nbOptions]);
    }
  }
  
  if(aMAJ & MAJ_FLECHE)
  {
    LCDGotoXY(0,oldSelect);
    LCDSend(' ',SEND_CHR);
    LCDGotoXY(0,select);
    LCDSend(0x7E,SEND_CHR);
  }
  
  // Menu principal
  if(menu->ecran == ECRAN_MENU_PRINCIPAL)
  {
    if(aMAJ & MAJ_REGL_ALARME)
    {  
      int y=(menu->nbOptions+MENU_ALARME1-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteHeure(&alarme);
      }
    }
    
    if(aMAJ & MAJ_REGL_ALARME2)
    {  
      int y=(menu->nbOptions+MENU_ALARME2-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteHeure(&alarme2);
      }
    }
    
    if(aMAJ & MAJ_REGL_SOLEIL)
    { 
      int y=(menu->nbOptions+MENU_SOLEIL-shiftMenu+1)%menu->nbOptions;
       if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteInt2(delaiSoleil);
        LCDWriteString(" min");
      }
    }
  }
  
  // Menu de reglage de la lumière
  if(menu->ecran == ECRAN_MENU_ECLAIRAGE)
  {
    if(aMAJ & MAJ_ECR_MIN)
    { 
      int y=(menu->nbOptions+MENU_ECRAN_MIN-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X+1 ,y);
        LCDWriteInt2(blMin);      
        LCDWriteString("/");
        LCDWriteInt2(FADE_COUNT-1);
      }
    }
    
    if(aMAJ & MAJ_ECR_MAX)
    { 
      int y=(menu->nbOptions+MENU_ECRAN_MAX-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X+1,y);
        LCDWriteInt2(blMax);
        LCDWriteString("/");
        LCDWriteInt2(FADE_COUNT-1);
      }
    }
  }
  
  // Menu de reglage de l'horloge
  if(menu->ecran == ECRAN_MENU_HEURE)
  {
    if(aMAJ & MAJ_REGL_HEURE)
    { 
      int y=(menu->nbOptions+MENU_HEURE_HEURE-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteHeure(&(date.heure));
      }
    }
    
    if(aMAJ & MAJ_REGL_DATE)
    { 
      int y=(menu->nbOptions+MENU_HEURE_DATE-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteDate(0);
      }
    }
    if(aMAJ & MAJ_REGL_JOURSEM)
    { 
      int y=(menu->nbOptions+MENU_HEURE_JOUR-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteJourSem();
      }
    }
    if(aMAJ & MAJ_REGL_AVANCEO2)
    { 
      int y=(menu->nbOptions+MENU_HEURE_AVANCE-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteInt2(avanceO2);
        LCDWriteString(" min");
      }
    }
  }
  
  // Menu de reglage du son
  if(menu->ecran == ECRAN_MENU_SON)
  {
    if(aMAJ & MAJ_REGL_SONNERIE)
    { 
      int y=(menu->nbOptions+MENU_SON_SONNERIE-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteString(strMusiques[numSonnerie]);
      }
    }
    
    if(aMAJ & MAJ_REGL_VOLMIN)
    { 
      int y=(menu->nbOptions+MENU_SON_VOLMIN-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteInt2(volMin);
      }
    }
    
    if(aMAJ & MAJ_REGL_VOLMAX)
    { 
      int y=(menu->nbOptions+MENU_SON_VOLMAX-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteInt2(volMax);
      }
    }
  }
  
  if(aMAJ & MAJ_CURS)
  if(cursOn)
  {
    LCDSend(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF, SEND_CMD);
  }
  else
    LCDSend(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF, SEND_CMD);
  LCDGotoXY(cursX,cursY);
  aMAJ = MAJ_RIEN;
}

void calcDebutAlarme()
{
  debutAlarme = alarme;
  int i;
  for(i=0;i<delaiSoleil;i++)
  {
    debutAlarme.minutes--;
    if(debutAlarme.minutes==-1)
    {
      debutAlarme.minutes = 59;
      debutAlarme.heures--;
      if(debutAlarme.heures ==-1)
        debutAlarme.heures = 23;
    }
  }
}

void reglageInt(int* num, int min, int max,int step,enum Maj type)
{
    aMAJ |= MAJ_CURS;
    cursOn=1;
    cursX = PARAMS_X+deltaX;
    cursY = select;
    if(type & (MAJ_ECR_MIN | MAJ_ECR_MAX) && (ecran == ECRAN_MENU_ECLAIRAGE))
      setBL(*num);
    while(btnAppuye != MENU)
    {
      if(aMAJ & (MAJ_ECR_MIN | MAJ_ECR_MAX) && (ecran == ECRAN_MENU_ECLAIRAGE))
        setBL(*num);
      if(aMAJ & (MAJ_REGL_VOLMIN | MAJ_REGL_VOLMAX) && (ecran == ECRAN_MENU_SON))
        beep(*num);
      if(aMAJ)
        affichageMenu();
      switch(btnAppuye){
        case HAUT :
          btnAppuye = AUCUN;
          *num += step;
          if(*num>max)
            *num-=1+max-min;
          aMAJ |= type;
          break;
        case BAS :
          btnAppuye = AUCUN;
          *num -= step;
           if(*num<min)
            *num+=max-min+1;
          aMAJ |= type;
          break;
      }
    }
    btnAppuye = AUCUN;
    if(type & (MAJ_ECR_MIN | MAJ_ECR_MAX) && (ecran == ECRAN_MENU_ECLAIRAGE))
      setBL(blFadeValue);
    aMAJ |= MAJ_CURS;
    cursOn=0;
}

void reglageDate(Date* d,enum Maj type)
{
    deltaX=1;
    reglageInt(&(d->jour),0,31,1,type);
    deltaX=4;
    reglageInt(&(d->mois),0,12,1,type);
    deltaX=9;
    reglageInt(&(d->annee),0,3000,1,type);
}


void reglageHeure(Heure* h,enum Maj type)
{
    deltaX=1;
    reglageInt(&(h->heures),0,23,1,type);
    deltaX=3;
    reglageInt(&(h->minutes),0,59,10,type);
    deltaX=4;
    reglageInt(&(h->minutes),0,59,1,type);
}

void setMenu(Menu* m)
{
  menu = m;
  shiftMenu = 0;
  select =1;
  oldSelect = 1;
  ecran = menu->ecran;
}

//gere les fleches haut-bas dans un menu
void gereMenu()
{
  if(aMAJ)
      affichageMenu();
    if(btnAppuye == HAUT)
    {
      btnAppuye=AUCUN;
      if(select>1)
      {
        oldSelect=select;
        select--;
        aMAJ|=MAJ_FLECHE;
      }
      else
      {
        shiftMenu = (shiftMenu + menu->nbOptions -1)%menu->nbOptions;
        aMAJ|=MAJ_MENU;
      }
    }
    if(btnAppuye == BAS)
    {
      btnAppuye = AUCUN;
      if(select<3)
      {
        oldSelect=select;
        select++;
        aMAJ |= MAJ_FLECHE;
      }
      else
      {
        shiftMenu = (shiftMenu+1)%menu->nbOptions;
        aMAJ|=MAJ_MENU;
      }
    }
}

void menuEclairage()
{
  setMenu(&mEclairage);
  aMAJ |= MAJ_MENU;
  while(1)
  {
    gereMenu(); //gere les fleches haut-bas
    if(btnAppuye == MENU)
    {
      btnAppuye = AUCUN;
      int selectionne = (shiftMenu+select-1)%menu->nbOptions;
      switch(selectionne)
      {
        case MENU_SORTIE:
          return;
          break;
        
        case MENU_ECRAN_MIN:
          deltaX=2;
          reglageInt(&blMin,0,FADE_COUNT-1,1,MAJ_ECR_MIN);
          break;
        
        case MENU_ECRAN_MAX:
          deltaX=2;
          reglageInt(&blMax,0,FADE_COUNT-1,1,MAJ_ECR_MAX);
          break;
      }
    }
  }
}

void menuHeure()
{
  setMenu(&mHeure);
  aMAJ |= MAJ_MENU;
  while(1)
  {
    gereMenu(); //gere les fleches haut-bas
    if(btnAppuye == MENU)
    {
      btnAppuye = AUCUN;
      int selectionne = (shiftMenu+select-1)%menu->nbOptions;
      switch(selectionne)
      {
        case MENU_SORTIE:
          return;
          break;
        
        case MENU_HEURE_HEURE:
          reglageHeure(&(date.heure),MAJ_REGL_HEURE);
          break;
          
        case MENU_HEURE_JOUR:
          deltaX=2;
          reglageInt(&(date.jourSem),0,6,1,MAJ_REGL_JOURSEM);
          break;
          
        case MENU_HEURE_AVANCE:
          deltaX=1;
          reglageInt(&avanceO2,0,99,1,MAJ_REGL_AVANCEO2);
          break;
          
        case MENU_HEURE_DATE:
          reglageDate(&date,MAJ_REGL_DATE);
          break;
      }
    }
  }
}

void menuSon()
{
  setMenu(&mSon);
  aMAJ |= MAJ_MENU;
  while(1)
  {
    gereMenu(); //gere les fleches haut-bas
    if(btnAppuye == MENU)
    {
      btnAppuye = AUCUN;
      int selectionne = (shiftMenu+select-1)%menu->nbOptions;
      switch(selectionne)
      {
        case MENU_SORTIE:
          return;
          break;
        
        case MENU_SON_SONNERIE:
          reglageInt(&numSonnerie,0,NBMUSIQUES-1,1,MAJ_REGL_SONNERIE);
          break;
        case MENU_SON_VOLMAX:
          //deltaX=2;
          reglageInt(&volMax,0,99,1,MAJ_REGL_VOLMAX);
          break;
          
        case MENU_SON_VOLMIN:
          //deltaX=1;
          reglageInt(&volMin,0,99,1,MAJ_REGL_VOLMIN);
          break;
      }
    }
  }
}


// Menu Principal
void menuPrincipal()
{
  setMenu(&mPrincipal);
  aMAJ |= MAJ_MENU;
  while(1)
  {
    gereMenu(); //gere les fleches haut-bas
    if(btnAppuye == MENU)
    {
      btnAppuye = AUCUN;
      int selectionne = (shiftMenu+select-1)%menu->nbOptions;
      switch(selectionne)
      {
        case MENU_SORTIE:
          return;
          break;
        case MENU_ALARME1:
          reglageHeure(&alarme,MAJ_REGL_ALARME);
          calcDebutAlarme();
          break;
        case MENU_ALARME2:
          reglageHeure(&alarme2,MAJ_REGL_ALARME2);
          break;
        case MENU_HEURE:
          menuHeure();  
          setMenu(&mPrincipal);
          aMAJ |= MAJ_MENU;
          break;
        case MENU_SOLEIL:
          deltaX=1;
          reglageInt(&delaiSoleil,0,99,1,MAJ_REGL_SOLEIL);
          calcDebutAlarme();
          break;
        case MENU_ECLAIRAGE:
          menuEclairage();  
          setMenu(&mPrincipal);
          aMAJ |= MAJ_MENU;
          break;
        case MENU_SON:
          menuSon();  
          setMenu(&mPrincipal);
          aMAJ |= MAJ_MENU;
          break;
        default:
          break;
      }
      
    }
  }
}


// Boucle principale du mode d'affichage de l'heure
void Loop()
{
  ecran = ECRAN_HEURE;
  // Si on doit mettre à jour quelque-chose, on Affiche
  if(aMAJ)
    affichageHeure();
  
  switch(btnAppuye)
  {
  case MENU:
    btnAppuye = AUCUN;
    menuPrincipal();
    ecran = ECRAN_HEURE;
    aMAJ = MAJ_CLEAR | MAJ_HEURE3 | MAJ_DATE | MAJ_DCF | MAJ_SOLEIL | MAJ_ALARME;
    break;
  
  // Dans le mode d'affichage de l'heure, le bouton haut definit si l'alarme est activée
  case HAUT:
    numAlarme = (numAlarme+1)%4;
    config &= ~(ALARM_ON | ALARM2_ON);
    if(numAlarme & 1)
      config |= ALARM_ON;
    if(numAlarme & 2)
      config |= ALARM2_ON;
      
    config &= ~SNOOZE;
    aMAJ |= MAJ_ALARME;
    btnAppuye = AUCUN;
    break;
    
  // Dans le mode d'affichage de l'heure, le bouton bas definit si le lever de soleil est activée
  case BAS:
    config ^= SOLEIL_ON;
    config &=~ LEVER_SOLEIL;
    soleilValue=0;
    setLamp(soleilValue);
    aMAJ |= MAJ_SOLEIL;
    btnAppuye = AUCUN;
    break;
  default:
    break;
  }
}


void enterLowPower()
{
  INV_OFF;
  DCF_POW_OFF;
  DCF_IE &= ~DCF_DAT;
  BTN_IE &= ~(BTN_MENU|BTN_HAUT|BTN_BAS);
  LCD_PORT &= ~(LCD_CLK | LCD_DAT | E);
  config &= ~(ALARM_ON | SOLEIL_ON | ALARM2_ON);
}

void exitLowPower()
{
  INV_ON;
  DCF_POW_ON;
  DCF_IE |= DCF_DAT;
  BTN_IE |= BTN_MENU|BTN_HAUT|BTN_BAS;
  aMAJ = MAJ_INIT | MAJ_CLEAR | MAJ_HEURE3 | MAJ_DATE | MAJ_DCF | MAJ_SOLEIL | MAJ_ALARME;
}

void main (void)
{ 
  // Timer watchdog, sert à compter le temps
  WDTCTL = WDTPW + WDTCNTCL + WDTTMSEL + WDTSSEL + WDTIS1;
  BCSCTL3 |= LFXT1S0 | XCAP_3;
  WDTcnt=0;
  // setup DCF
  DCF_DIR |= DCF_PON;
  //DCF_DIR &= ~DCF_DAT;    //par defaut
  
  // resistance pull-up dcf 
  DCF_REN |= DCF_DAT;
  DCF_OUT |= DCF_DAT;
  // dcf power on il faut mettre le pin à 0 pour l'activer
  DCF_POW_ON;  
  DCF_IE |= DCF_DAT;    // interruption activée
  DCF_IES |= DCF_DAT;   // high to low transition
  
  LCD_DIR |= (LCD_CLK | LCD_DAT | E);
  
  // resistances pull-up boutons
  BTN_REN |= BTN_MENU|BTN_HAUT|BTN_BAS;
  BTN_OUT |= BTN_MENU|BTN_HAUT|BTN_BAS;
  
  // interruption sur les boutons
  BTN_IES |= BTN_MENU|BTN_HAUT|BTN_BAS; // high to low transition
  BTN_IE |= BTN_MENU|BTN_HAUT|BTN_BAS;  // interruptions activées
    
  IE1 |= WDTIE;
  WRITE_SR(GIE); 		            //Enable global interrupts
  BCSCTL1 = CALBC1_1MHZ;        // run at 1Mhz
  DCOCTL = CALDCO_1MHZ;

  LCDInit();          //Initialise l'écran

  // PWM bl
  BL_DIR |= BL_PIN;
  BL_SEL |= BL_PIN;
  TA1CCR0 = BL_FADE_MAX;
  TA1CCTL2 = OUTMOD_6;
  TA1CTL = TASSEL_2 | MC_3 ;   //Selectionne l'horloge rapide et mode up/down
  
  // PWM Lampe
  LAMP_DIR |= LAMP_PIN;
  LAMP_SEL |= LAMP_PIN;
  TA1CCTL1 = OUTMOD_6;
  
  // sortie inverseur
  INV_DIR |= INV_PIN;
  INV_SEL |= INV_PIN;
  INV_ON;

  
  // Haut parleur
  HP_DIR |= HP_PIN;
  HP_SEL |= HP_PIN;
  TA0CTL = TASSEL_2 | MC_1;
  
  // Alimentation externe
  if(POW_IN & POW_PIN)
  {
    POW_IES |= POW_PIN; // high to low transition
  }
  else
  {
    POW_IES &= ~POW_PIN; // low to high transition
    enterLowPower();
    LPM3;
  }
  POW_IE  |= POW_PIN;  // interruptions activées
  
  
  config =0;
  
  HPOff;
  iMusique = 0;
  musiqueCnt = 0;
  volMax = 70;
  numSonnerie = 1;

  // retro-éclairage
  blMin = 0;
  blMax = FADE_COUNT-1;
  setBL(blMin);
  blTimer = 0;
  
  // parametres par défaut
  date.jour = 31;
  date.mois = 12;
  date.annee = 2012;
  date.jourSem = 1;
  avanceO2 = 0;
  
  deltaX=0;
  
  date.heure.heures = 10;
  date.heure.minutes = 30;
  date.heure.secondes = 0;                                                           
  delaiSoleil = 25;
  
  // config |= ALARM_ON;
  
  // definition des menus
  // Menu principal
  mPrincipal.titre = strReglage;
  mPrincipal.nbOptions = 7;
  mPrincipal.strOptions = optsMenuPrincipal;
  mPrincipal.ecran = ECRAN_MENU_PRINCIPAL;
  
  // Menu Eclairage
  mEclairage.titre = strEclairage+1;    //+1 pour supprimer l'espace du début
  mEclairage.nbOptions = 4;
  mEclairage.strOptions = optsMenuEclairage;
  mEclairage.ecran = ECRAN_MENU_ECLAIRAGE;
  
  // Menu Heure
  mHeure.titre = strHorloge;
  mHeure.nbOptions = 5;
  mHeure.strOptions = optsMenuHeure;
  mHeure.ecran = ECRAN_MENU_HEURE;
  
  //Menu sons
  mSon.titre = strSon;
  mSon.nbOptions = 4;
  mSon.strOptions = optsMenuSon;
  mSon.ecran = ECRAN_MENU_SON;  
  
  aMAJ = MAJ_HEURE3 | MAJ_DATE | MAJ_DCF | MAJ_SOLEIL | MAJ_ALARME;
  
  setLamp(soleilValue=0);
        
  while(1)
  {
    Loop();
  }
}

void ajouteMinutes(int minutes)
{
  date.heure.minutes+=minutes;
  if(date.heure.minutes>60)
  {
   date.heure.minutes=date.heure.minutes%60;
   date.heure.heures++;
   if(date.heure.heures==24)
   {
    date.heure.heures=0;
    date.jour++;
    if(date.jourSem<6)
      date.jourSem++;
    else
      date.jourSem=0;
    if(date.jour>jourDansMois[date.mois-1])        
    {
      date.jour=1;
      date.mois++;
      if(date.mois==13)
      {
        date.mois=1;
        date.annee++;
      }
     }
   }
  }
}

// Port 2 interrupt service routine
interrupt (PORT1_VECTOR) Port_1(void)
{
    if(P1IFG & DCF_DAT)
    {
      DCFBit++;       //prochain bit
      //debug = DCFErreur;
      if(DCFCnt<50)
      {
        DCFErreur = 1;
      }
      
      if(DCFCnt>100)
      {
         if((DCFBit==59) && !DCFErreur)
         {
          date = DCFDate;
          ajouteMinutes(avanceO2);
          date.jourSem = (date.jourSem+6)%7;
          aMAJ |= MAJ_HEURE3 | MAJ_DATE;
          DCF_POW_OFF; // On est mis à jour, on éteint le DCF
          config |= DCF_ON;
         }
         DCFBit = 0;
         DCFDate.heure.minutes = DCFDate.heure.heures = DCFDate.heure.secondes = DCFDate.jourSem = DCFDate.jour = DCFDate.mois = DCFErreur = DCFParite =0;  
         DCFDate.annee = 2000;   
      } 
      DCFCnt = 0;
      blinkDCF();
      //aMAJ |= MAJ_DEBUG;
      P1IFG &= ~DCF_DAT;
      DCF_IE &= ~DCF_DAT;
    }
    
    if(P1IFG & POW_PIN)
    {
      if(POW_IES & POW_PIN)
      {
        POW_IES &= ~POW_PIN;
        enterLowPower();
        _BIS_SR_IRQ(LPM3_bits + GIE);
      }
      else
      {  
        POW_IES |= POW_PIN;
        exitLowPower();
        LPM3_EXIT;
      }
      P1IFG &= ~POW_PIN;
    }
}


// Port 2 interrupt service routine
interrupt (PORT2_VECTOR) Port_2(void)
{        
    if(P2IFG & (BTN_MENU | BTN_HAUT| BTN_BAS))
    {
      if(config & JOUE_MUSIQUE)
      {
          stopMusique();
          config |= SNOOZE;
          timerSnooze = DELAI_SNOOZE;
      }
      //si pas de bl on ne prend pas en compte le bouton
      if(blTimer == 0)
      {
        P2IFG &=~(BTN_MENU | BTN_HAUT| BTN_BAS);
        blFadeStep = BL_FADE_STEP;
        blFadeValue = blMin;
        config |= BL_FADE;
      }
      blTimer=5; //5 secondes
    }
    if(P2IFG & BTN_MENU)
    {
        btnAppuye = MENU;
        P2IFG &=~BTN_MENU;
    }
    if(P2IFG & BTN_HAUT)
    {
         btnAppuye = HAUT; 
         P2IFG &=~BTN_HAUT;   
    }
    if(P2IFG & BTN_BAS)
    {
      btnAppuye = BAS;
      P2IFG &=~BTN_BAS;
    }  
}

// Watchdog interrupt service routine
interrupt (WDT_VECTOR) Watchdog(void)
{
  WDTcnt++; // il faut compter 64 fois pour une seconde.
  DCFCnt++; // Il faut compter 9 fois (=156 ms) avant de vérifier DCF : si signal=low->1 sinon ->0 
  
  if(config & FLASH_LIGHT)
  {
    flashCnt++;
    if(flashCnt == 192)
    {
      flashCnt=0;
      config &= ~FLASH_LIGHT;
      setBL(blMin);
    }
    if(flashCnt%8==4)
    {
      setBL(59);
      setLamp(59);
    }
    if(flashCnt%8==0)
    {
      setBL(0);
      setLamp(0);
    }
  }
  
  if(config & JOUE_MUSIQUE)
  {
    musiqueCnt--;
    if(musiqueCnt==2)
      HPOff;
    if(musiqueCnt==0)
    {
      HPOn;
      musiqueCnt = musique[iMusique++];  // duree de la note
      int note = musique[iMusique++];
      int octave = musique[iMusique++];
      setFreqHP(note,octave,volume);     // frequence prenant en compte l'octave
      if(volume<volMax) volume++;
      if(iMusique == nMusique)
      {
        config |= SNOOZE;
        timerSnooze = DELAI_SNOOZE;
        stopMusique();
      }
    }
  }
  
  if(DCFCnt == 32)
  {
    DCF_IE |= DCF_DAT;
    P1IFG &= ~DCF_DAT;
  }
        
  
  // Il faut vérifier DCF
  if(DCFCnt==9)
  {
    int bit = !(DCF_IN & DCF_DAT);
    DCFParite ^=bit;
    switch(DCFBit)
    {
      case 20 : 
        if(!bit) DCFErreur = 1;   // doit toujours etre à 1
        DCFParite = 0;
        break;
      case 21 :
        if(bit) DCFDate.heure.minutes+=1;
        break;
      case 22 :
        if(bit) DCFDate.heure.minutes+=2;
        break;
      case 23 :
        if(bit) DCFDate.heure.minutes+=4;
        break;
      case 24 :
        if(bit) DCFDate.heure.minutes+=8;
        break;
      case 25 :
        if(bit) DCFDate.heure.minutes+=10;
        break;
      case 26 :
        if(bit) DCFDate.heure.minutes+=20;
        break;
      case 27 :
        if(bit) DCFDate.heure.minutes+=40;
        break;
      case 28 :
        DCFErreur |= DCFParite;
        break;
      case 29 :
        if(bit) DCFDate.heure.heures+=1;
        break;
      case 30 :
        if(bit) DCFDate.heure.heures+=2;
        break;
      case 31 :
        if(bit) DCFDate.heure.heures+=4;
        break;
      case 32 :
        if(bit) DCFDate.heure.heures+=8;
        break;
      case 33 :
        if(bit) DCFDate.heure.heures+=10;
        break;
      case 34 :
        if(bit) DCFDate.heure.heures+=20;
        break;
      case 35 :
        DCFErreur |= DCFParite;
        break;
      case 36 :
        if(bit) DCFDate.jour+=1;
        break;
      case 37 :
        if(bit) DCFDate.jour+=2;
        break;
      case 38 :
        if(bit) DCFDate.jour+=4;
        break;
      case 39 :
        if(bit) DCFDate.jour+=8;
        break;
      case 40 :
        if(bit) DCFDate.jour+=10;
        break;
      case 41 :
        if(bit) DCFDate.jour+=20;
        break;
      case 42 :
        if(bit) DCFDate.jourSem+=1;
        break;
      case 43 :
        if(bit) DCFDate.jourSem+=2;
        break;
      case 44 :
        if(bit) DCFDate.jourSem+=4;
        break;
      case 45 :
        if(bit) DCFDate.mois+=1;
        break;
      case 46 :
        if(bit) DCFDate.mois+=2;
        break;
      case 47 :
        if(bit) DCFDate.mois+=4;
        break;
      case 48 :
        if(bit) DCFDate.mois+=8;
        break;
      case 49 :
        if(bit) DCFDate.mois+=10;
        break;
      case 50 :
        if(bit) DCFDate.annee+=1;
        break;
      case 51 :
        if(bit) DCFDate.annee+=2;
        break;
      case 52 :
        if(bit) DCFDate.annee+=4;
        break;
      case 53 :
        if(bit) DCFDate.annee+=8;
        break;
      case 54 :
        if(bit) DCFDate.annee+=10;
        break;
      case 55 :
        if(bit) DCFDate.annee+=20;
        break;
      case 56 :
        if(bit) DCFDate.annee+=40;
        break;
      case 57 :
        if(bit) DCFDate.annee+=80;
        break;
      case 58 :
        DCFErreur |= DCFParite;
        break;
    }
    
  }
  
  if(config & BL_FADE)
  {
    blFadeValue+=blFadeStep;
    if(blFadeValue>blMax)
    {
      blFadeValue=blMax;
      config &= ~BL_FADE;
    }
    if(blFadeValue<blMin)
    {
      blFadeValue=blMin;
      config &= ~BL_FADE;
    }
    setBL(blFadeValue);
  }
  
  if(WDTcnt==64)
  {
    WDTcnt=0;
    date.heure.secondes++;
    if(ecran == ECRAN_HEURE)
          aMAJ |= MAJ_SECONDES;
    if(config & LEVER_SOLEIL)
    {
      soleilTimer++;
      if(soleilTimer>=delaiSoleil)
      {
        soleilTimer=0;
        soleilValue+=1;
        if(soleilValue > FADE_COUNT-1)
        {
          soleilValue = FADE_COUNT-1;
          config &= ~LEVER_SOLEIL; 
        }
        setLamp(soleilValue);
        setBL(soleilValue);
      }
    }
    
    if(blTimer>0)
      if(--blTimer==0)
      {
        config |= BL_FADE;
        blFadeStep = -BL_FADE_STEP;
      }
    if(config & SNOOZE)
    {
      if(--timerSnooze==0)
      {
        config &= ~SNOOZE;
        sonneAlarme();
      }
    }
    if(date.heure.secondes==60)
    {
      date.heure.secondes=0;
      date.heure.minutes++;
      if(date.heure.minutes==60)
      {
       date.heure.minutes=0;
       date.heure.heures++;
       if(date.heure.heures==24)
       {
        date.heure.heures=0;
        DCF_POW_ON;           //à minuit on met à jour l'heure
        date.jour++;
        if(date.jourSem<6)
          date.jourSem++;
        else
          date.jourSem=0;
        if(date.jour>jourDansMois[date.mois-1])        
        {
          date.jour=1;
          date.mois++;
          if(date.mois==13)
          {
            date.mois=1;
            date.annee++;
          }
        }
        if(ecran == ECRAN_HEURE)
          aMAJ |= MAJ_DATE;
       }
      }
      if(ecran == ECRAN_HEURE)
        aMAJ|= MAJ_HEURE3;
        
      // Est-ce que le soleil doit se lever ?
      if(config & SOLEIL_ON) 
        if((date.heure.heures == debutAlarme.heures) && (date.heure.minutes == debutAlarme.minutes))
        {
          config |= LEVER_SOLEIL; 
        }
      // Est-ce que l'alarme1 doit sonner ?
      if(config & ALARM_ON) 
        if((date.heure.heures == alarme.heures) && (date.heure.minutes == alarme.minutes))
        {
          sonneAlarme();
        }
      // Est-ce que l'alarme doit sonner ?
      if(config & ALARM2_ON) 
        if((date.heure.heures == alarme2.heures) && (date.heure.minutes == alarme2.minutes))
        {
          sonneAlarme();
        }
      // 3h18 ?
      if((date.heure.heures == 3) && (date.heure.minutes == 18))
        {
          config |= FLASH_LIGHT; 
        }
    }
  }
}
