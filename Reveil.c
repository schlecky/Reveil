#include <legacymsp430.h>
#include <msp430g2553.h>

#include "reveil.h"
#include "hd77480.h"

Menu mPrincipal, mEclairage, mHeure;
Menu* menu;
Heure alarme, debutAlarme;
Date date;
//int annee, mois, jour, jourSem;
int deltaX;
int delaiSoleil; // temps de lever du soleil en minutes
int avanceO2;    // avance en minutes spécifique 02
int WDTcnt, DCFCnt, interTimer, soleilTimer, soleilValue, blTimer, blFadeValue, blFade, blFadeStep;
int blMax, blMin;
int cursOn, cursX, cursY;
int select, oldSelect, shiftMenu;
int iMusique, musiqueCnt;
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

void LCDWriteString(unsigned char* str)
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
 /*
  LCDSendBigNum3(debutAlarme.heures/10,0);
  LCDSendBigNum3(debutAlarme.heures%10,5);
  LCDSendBigNum3(debutAlarme.minutes/10,10);
  LCDSendBigNum3(debutAlarme.minutes%10,15);*/
  //double points
  LCDGotoXY(9,1);
  LCDSend(3,SEND_CHR);
  LCDGotoXY(9,2);
  LCDSend(3,SEND_CHR);
}




// definie l'intensité de l'éclairage de l'écran, bl est entre 0 et BL_FADE_MAX
void setBL(int bl)
{
  TA1CCR2 = BL_FADE_MAX-bl;
}

// definie l'intensité de l'éclairage du soleil, intens est entre 0 et LAMP_FADE_MAX
void setLamp(int intens)
{
  TA1CCR1 = LAMP_FADE_MAX-pwm[intens];
  //TA0CCR1 = LAMP_FADE_MAX-intens;
}

// define la frequence du haut parleur
void setFreqHP(int freq, int vol)
{
  TA0CCR0 = frequence[freq];
  TA0CCR1 = TA0CCR0 - vol;
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
  LCDGotoXY(19,1);
  if(config & ALARM_ON)
    LCDSend(4,SEND_CHR);
  else
    LCDSend(' ',SEND_CHR);
}

// Met a jour le symbole du soleil
void dispSoleil()
{
  LCDGotoXY(19,2);
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
  if(aMAJ & MAJ_CLEAR)
    LCDClear();        
  
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
        aMAJ |= MAJ_REGL_ALARME | MAJ_REGL_SOLEIL | MAJ_FLECHE ;
        break;
      case ECRAN_MENU_ECLAIRAGE:
        aMAJ |= MAJ_ECR_MIN | MAJ_ECR_MAX | MAJ_FLECHE;
        break;
      case ECRAN_MENU_HEURE:
        aMAJ |= MAJ_REGL_HEURE | MAJ_REGL_DATE | MAJ_REGL_JOURSEM | MAJ_REGL_AVANCEO2 | MAJ_FLECHE ;
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
      int y=(menu->nbOptions+MENU_ALARME-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteHeure(&alarme);
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
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteInt3(blMin);
        LCDWriteString(" %");
      }
    }
    
    if(aMAJ & MAJ_ECR_MAX)
    { 
      int y=(menu->nbOptions+MENU_ECRAN_MAX-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X,y);
        LCDWriteInt3(blMax);
        LCDWriteString(" %");
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
    if(type & (MAJ_ECR_MIN | MAJ_ECR_MAX)& (ecran==ECRAN_MENU_ECLAIRAGE))
      setBL(*num);
    while(btnAppuye != MENU)
    {
      if(aMAJ & (MAJ_ECR_MIN | MAJ_ECR_MAX) & (ecran==ECRAN_MENU_ECLAIRAGE))
        setBL(*num);
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
    if(type & (MAJ_ECR_MIN | MAJ_ECR_MAX)& (ecran==ECRAN_MENU_ECLAIRAGE))
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
          reglageInt(&blMin,0,100,1,MAJ_ECR_MIN);
          break;
        
        case MENU_ECRAN_MAX:
          deltaX=2;
          reglageInt(&blMax,0,100,1,MAJ_ECR_MAX);
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
        case MENU_ALARME:
          reglageHeure(&alarme,MAJ_REGL_ALARME);
          calcDebutAlarme();
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
    config ^= ALARM_ON;
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

/*
void __low_level_init()
{
  WDTCTL = WDTPW + WDTHOLD;
} 
*/

void main (void)
{ 
/*
  WDTCTL = WDTPW + WDTHOLD;     // Stop watchdog timer
  P1DIR = 0xFF;
  LCDSend4Bits(0xFF);
  */

  // Timer watchdog, sert à compter le temps
  WDTCTL = WDTPW + WDTCNTCL + WDTTMSEL + WDTSSEL + WDTIS1;
  BCSCTL3 |= LFXT1S0 | XCAP_3;
  WDTcnt=0;

  P1DIR = 0xFF;                        // Port1 = Output
  //P2DIR = BIT0 | BIT2;                 // Port2 = Output pour DCF powerOn et BL
  // resistances pull-up boutons
  BTN_REN |= BTN_MENU|BTN_HAUT|BTN_BAS;
  BTN_OUT |= BTN_MENU|BTN_HAUT|BTN_BAS;
  
  // resistance pull-up dcf 
  DCF_REN |= DCF_DAT;
  DCF_OUT |= DCF_DAT;
  
  IE1 |= WDTIE;
  WRITE_SR(GIE); 		            //Enable global interrupts
  BCSCTL1 = CALBC1_1MHZ;        // run at 1Mhz
  DCOCTL = CALDCO_1MHZ;

  LCDInit();          //Initialise l'écran

  // dcf power on il faut mettre le pin à 0 pour l'activer
  DCF_OUT &=~ DCF_PON;
  
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
  //TA0CTL = TASSEL_2 | MC_3;  //Selectionne l'horloge rapide et mode up/down
  
  // sortie inverseur
  INV_DIR |= INV_PIN;
  INV_SEL |= INV_PIN;
  TA1CCTL0 = OUTMOD_4;
  
  // Haut parleur
  HP_DIR |= HP_PIN;
  HP_SEL |= HP_PIN;
  TA0CTL = TASSEL_2 | MC_1;
  
  config =0;
  
  HPOff;
  iMusique = 0;
  musiqueCnt = 0;
  config |= JOUE_MUSIQUE;
  
  //setFreqHP(1,50);
  HPOn;
  
  
  // interruption sur les boutons
  BTN_IES |= BTN_MENU|BTN_HAUT|BTN_BAS; // high to low transition
  BTN_IE |= BTN_MENU|BTN_HAUT|BTN_BAS;  // interruptions activées
  
  DCF_IE |= DCF_DAT;    // interruption activée
  DCF_IES |= DCF_DAT;   // high to low transition

  
  
  // retro-éclairage
  blMin = 3;
  blMax = BL_FADE_MAX;
  setBL(blMin);
  
  // parametres par défaut
  date.jour = 31;
  date.mois = 12;
  date.annee = 1983;
  date.jourSem = 1;
  avanceO2 = 3;
  
  deltaX=0;
  
  date.heure.heures = 23;
  date.heure.minutes = 58;
  delaiSoleil = 3;
  
  // definition des menus
  // Menu principal
  mPrincipal.titre = strReglage;
  mPrincipal.nbOptions = 5;
  mPrincipal.strOptions = optsMenuPrincipal;
  mPrincipal.ecran = ECRAN_MENU_PRINCIPAL;
  
  // Menu Eclairage
  mEclairage.titre = strEclairage+1;    //+1 pour supprimer l'espace du début
  mEclairage.nbOptions = 5;
  mEclairage.strOptions = optsMenuEclairage;
  mEclairage.ecran = ECRAN_MENU_ECLAIRAGE;
  
  // Menu Heure
  mHeure.titre = strHorloge;
  mHeure.nbOptions = 5;
  mHeure.strOptions = optsMenuHeure;
  mHeure.ecran = ECRAN_MENU_HEURE;
  
  aMAJ = MAJ_HEURE3 | MAJ_DATE | MAJ_DCF | MAJ_SOLEIL | MAJ_ALARME;
  
  setLamp(soleilValue=0);
  
  while(1)
  {
    Loop();
  }
}

// Timer A0 interrupt service routine.
/*
interrupt (TIMERA0_VECTOR) Timer_A (void)
{ 
  
}
*/


// Port 2 interrupt service routine
interrupt (PORT2_VECTOR) Port_2(void)
{        
    if(P2IFG & (BTN_MENU | BTN_HAUT| BTN_BAS))
    {
      interTimer = DELAI_INTER;
      //si pas de bl on ne prend pas en compte le bouton 
      if(blTimer == 0)
      {
        P2IFG &=~(BTN_MENU | BTN_HAUT| BTN_BAS);
        BTN_IE &=~(BTN_MENU|BTN_HAUT|BTN_BAS);
        blFadeStep = BL_FADE_STEP;
        blFadeValue = blMin;
        blFade =  1;
      }
      blTimer=5; //5 secondes
    }
    if(P2IFG & BTN_MENU)
    {
        BTN_IE &=~(BTN_MENU|BTN_HAUT|BTN_BAS);
        btnAppuye = MENU;
        P2IFG &=~BTN_MENU;
    }
    if(P2IFG & BTN_HAUT)
    {
      BTN_IE &=~(BTN_MENU|BTN_HAUT|BTN_BAS);
         btnAppuye = HAUT; 
         P2IFG &=~BTN_HAUT;   
    }
    if(P2IFG & BTN_BAS)
    {
      BTN_IE &=~(BTN_MENU|BTN_HAUT|BTN_BAS);
      btnAppuye = BAS;
      P2IFG &=~BTN_BAS;
    }  
    if(P2IFG & DCF_DAT)
    {
      blinkDCF();
      P2IFG &= ~DCF_DAT;
    }
}

// Port 1 interrupt service routine
interrupt (WDT_VECTOR) Watchdog(void)
{
  WDTcnt++; // il faut compter 64 fois pour une seconde.
  DCFCnt++; // Il faut compter 9 fois (=156 ms) avant de vérifier DCF : si signal=low->1 sinon ->0 
  
  if(config & JOUE_MUSIQUE)
  {
    musiqueCnt--;
    if(musiqueCnt==0)
    {
      musiqueCnt = musique_tempo[iMusique];
      setFreqHP(musique[iMusique],20);
      if(++iMusique == NMUSIQUE)
      {
        config &= ~JOUE_MUSIQUE;
        HPOff;
      }
    }
  }
  
  // Il faut vérifier DCF
  if(DCFCnt==9)
  {
  }
  
  // gère l'anti-rebond des boutons
  if(interTimer)
  {
    if(--interTimer==0)
    {
      BTN_IFG=0;
      BTN_IE |= BTN_MENU|BTN_HAUT|BTN_BAS;
    }
  }
  
  if(blFade == 1)
  {
    blFadeValue+=blFadeStep;
    if(blFadeValue>blMax)
    {
      blFadeValue=blMax;
      blFade = 0;
    }
    if(blFadeValue<blMin)
    {
      blFadeValue=blMin;
      blFade = 0;
    }
    setBL(blFadeValue);
  }
  
  if(WDTcnt==64)
  {
    WDTcnt=0;
    date.heure.secondes++;
    if(config & LEVER_SOLEIL)
    {
      soleilTimer++;
      if(soleilTimer>delaiSoleil)
      {
        soleilTimer=0;
        soleilValue+=1;
        if(soleilValue > FADE_COUNT-1)
        {
          soleilValue = FADE_COUNT-1;
          config &= ~LEVER_SOLEIL; 
        }
        setLamp(soleilValue);

      }
    }
    
    if(blTimer>0)
      if(--blTimer==0)
      {
        blFade=1;
        blFadeStep = -BL_FADE_STEP;
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
        if((date.heure.heures == debutAlarme.heures) & (date.heure.minutes == debutAlarme.minutes))
        {
          config |= LEVER_SOLEIL; 
        }
    }
  }
}
