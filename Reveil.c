#include <legacymsp430.h>
#include <msp430g2553.h>

#include "reveil.h"
#include "hd77480.h"

Menu mPrincipal, mEclairage;
Menu* menu;

int annee, mois, jour, jourSem;
int delaiSoleil; //temps de lever du soleil en minutes
int WDTcnt, DCFCnt, interTimer, blTimer, blFadeValue, blFade, blFadeStep;
int maj;
int blMax, blMin;
int cursOn, cursX, cursY;
int select, oldSelect, shiftMenu;
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

void LCDWriteHeure(Heure heure)
{
  LCDWriteInt2(heure.heures);
  LCDSend(':',SEND_CHR);
  LCDWriteInt2(heure.minutes);
}

void LCDWriteDate()
{
  LCDGotoXY(0,3);  
  int i;
  for(i=0;i<3;i++)
    LCDSend(nomJours[3*jourSem+i],SEND_CHR);
  LCDSend(' ',SEND_CHR);
  LCDWriteInt2(jour);
  LCDSend('/',SEND_CHR);
  LCDWriteInt2(mois);
  LCDSend('/',SEND_CHR);
  LCDWriteInt4(annee);
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
  LCDSendBigNum3(heure.heures/10,0);
  LCDSendBigNum3(heure.heures%10,5);
  LCDSendBigNum3(heure.minutes/10,10);
  LCDSendBigNum3(heure.minutes%10,15);
  
  //double points
  LCDGotoXY(9,1);
  LCDSend(3,SEND_CHR);
  LCDGotoXY(9,2);
  LCDSend(3,SEND_CHR);
}


void LCDInit(void)
{ 
  LCD_PORT &= ~(RS|E);
  
  // we start in 8bit mode, try to set 4 bit mode
  LCDWrite4Bits(0x03);
  Delay(15000);
  LCDWrite4Bits(0x03);
  Delay(500);
  LCDWrite4Bits(0x03);
  Delay(500);
  LCDWrite4Bits(0x02);
  Delay(1000);
  
  LCDSend(0x28,SEND_CMD);
  Delay(100);
  LCDSend(0x08,SEND_CMD);
  Delay(100);
  LCDSend(0x01,SEND_CMD);
  Delay(100);  
  LCDSend(0x06,SEND_CMD);
  Delay(100); 
  LCDSend(0x0C,SEND_CMD);
  Delay(100); 

  LCDSend(LCD_FUNCTIONSET |LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS, SEND_CMD);
  Delay(100);
  LCDSend(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF, SEND_CMD);
  Delay(100);
  LCDSend(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT, SEND_CMD);

  LCDSendCustomChar();
}

// definie l'intensité de l'éclairage de l'écran, bl est entre 0 et BL_FADE_MAX
void setBL(int bl)
{
  TA1CCR1 = BL_FADE_MAX-bl;
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
    LCDSend(' ',SEND_CHR);
  else
    LCDSend(4,SEND_CHR);
}

// Met a jour le symbole du soleil
void dispSoleil()
{
  LCDGotoXY(19,2);
  if(config & SOLEIL_ON)
    LCDSend(' ',SEND_CHR);
  else
    LCDSend(6,SEND_CHR);
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
    LCDWriteDate();

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
        aMAJ |= MAJ_REGL_HEURE | MAJ_REGL_ALARME | MAJ_REGL_SOLEIL | MAJ_FLECHE ;
        break;
      case ECRAN_MENU_ECLAIRAGE:
        aMAJ |= MAJ_ECR_MIN | MAJ_ECR_MAX | MAJ_FLECHE;
        break;
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
  
  if(menu->ecran == ECRAN_MENU_PRINCIPAL)
  {
    if(aMAJ & MAJ_REGL_HEURE)
    { 
      int y=(menu->nbOptions+MENU_HEURE-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteHeure(heure);
      }
    }
    
    if(aMAJ & MAJ_REGL_ALARME)
    {  
      int y=(menu->nbOptions+MENU_ALARME-shiftMenu+1)%menu->nbOptions;
      if((y>0) & (y<4))
      {
        LCDGotoXY(PARAMS_X ,y);
        LCDWriteHeure(alarme);
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

void reglageInt(int* num, int min, int max,enum Maj type)
{
    aMAJ |= MAJ_CURS;
    cursOn=1;
    cursX = PARAMS_X;
    cursY = select;
    if(type & (MAJ_ECR_MIN | MAJ_ECR_MAX))
      setBL(*num);
    while(btnAppuye != MENU)
    {
      if(aMAJ & (MAJ_ECR_MIN | MAJ_ECR_MAX))
        setBL(*num);
      if(aMAJ)
        affichageMenu();
      switch(btnAppuye){
        case HAUT :
          btnAppuye = AUCUN;
          if(*num<max)
            *num += 1;
          aMAJ |= type;
          break;
        case BAS :
          btnAppuye = AUCUN;
          if(*num>min)
          *num -= 1;
          aMAJ |= type;
          break;
      }
    }
    btnAppuye = AUCUN;
    if(type & (MAJ_ECR_MIN | MAJ_ECR_MAX))
      setBL(blFadeValue);
    aMAJ |= MAJ_CURS;
    cursOn=0;
}

void reglageHeure(Heure* h,enum Maj type)
{
    aMAJ |= MAJ_CURS;
    cursOn=1;
    cursX = PARAMS_X+1;
    cursY = select;
    while(btnAppuye != MENU)
    {
      if(aMAJ)
        affichageMenu();
      switch(btnAppuye){
        case HAUT :
          btnAppuye = AUCUN;
          h->heures = (h->heures+1)%24;
          aMAJ |= type;
          break;
        case BAS :
          btnAppuye = AUCUN;
          h->heures = (h->heures+23)%24;
          aMAJ |= type;
          break;
      }
    }
    aMAJ |= MAJ_CURS;
    btnAppuye = AUCUN;
    cursX = PARAMS_X+3;
    while(btnAppuye != MENU)
    {
    if(aMAJ)
      affichageMenu();
    switch(btnAppuye){
      case HAUT :
        btnAppuye = AUCUN;
        h->minutes = (h->minutes+10)%60;
        aMAJ |= type;
        break;
      case BAS :
        btnAppuye = AUCUN;
        h->minutes = (h->minutes+50)%60;
        aMAJ |= type;
        break;
      }
    }
    cursX = PARAMS_X+4;
    btnAppuye = AUCUN;
    aMAJ |= MAJ_CURS;
    while(btnAppuye != MENU)
    {
    if(aMAJ)
      affichageMenu();
    switch(btnAppuye){
      case HAUT :
        btnAppuye = AUCUN;
        h->minutes = h->minutes-h->minutes%10 + (h->minutes+1)%10;
        aMAJ |= type;
        break;
      case BAS :
        btnAppuye = AUCUN;
        h->minutes = h->minutes-h->minutes%10 + (h->minutes+9)%10;
        aMAJ |= type;
        break;
      }
    }
    btnAppuye = AUCUN;
    aMAJ |= MAJ_CURS;
    cursOn=0;
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
      btnAppuye = AUCUN;
      int selectionne = (shiftMenu+select-1)%menu->nbOptions;
      switch(selectionne)
      {
        case MENU_SORTIE:
          return;
          break;
        
        case MENU_ECRAN_MIN:
          reglageInt(&blMin,0,100,MAJ_ECR_MIN);
          break;
        
        case MENU_ECRAN_MAX:
          reglageInt(&blMax,0,100,MAJ_ECR_MAX);
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
          break;
        case MENU_HEURE:
          reglageHeure(&heure,MAJ_REGL_HEURE); 
          break;
        case MENU_SOLEIL:
          reglageInt(&delaiSoleil,0,99,MAJ_REGL_SOLEIL);
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
    //alarmSet = !alarmSet;
    aMAJ |= MAJ_ALARME;
    btnAppuye = AUCUN;
    break;
    
  // Dans le mode d'affichage de l'heure, le bouton bas definit si le lever de soleil est activée
  case BAS:
    //soleilSet = !soleilSet;
    config ^= SOLEIL_ON;
    aMAJ |= MAJ_SOLEIL;
    btnAppuye = AUCUN;
    break;
  default:
    break;
  }
}

void main (void)
{ 
  // Timer watchdog, sert à compter le temps
  WDTCTL = WDTPW + WDTHOLD;     // Stop watchdog timer
  WDTCTL = WDTPW + WDTCNTCL + WDTTMSEL + WDTSSEL + WDTIS1;
  BCSCTL3 |= LFXT1S0 | XCAP_3;
  WDTcnt=0;

  P1DIR = 0xFF;                        // Port1 = Output
  P2DIR = BIT0 | BIT2;                 // Port2 = Output pour DCF powerOn et BL
  // resistances pull-up boutons
  BTN_REN |= BTN_MENU|BTN_HAUT|BTN_BAS;
  BTN_OUT |= BTN_MENU|BTN_HAUT|BTN_BAS;
  
  // resistance pull-up dcf 
  DCF_REN |= DCF_DAT;
  DCF_OUT |= DCF_DAT;
  
  BL_DIR |= BL_PIN;
  
  IE1 |= WDTIE;
  WRITE_SR(GIE); 		            //Enable global interrupts
  BCSCTL1 = CALBC1_1MHZ;        // run at 1Mhz
  DCOCTL = CALDCO_1MHZ;

  LCDInit();          //Initialise l'écran
    
  // dcf power on il faut mettre le pin à 0 pour l'activer
  DCF_OUT &=~ DCF_PON;
  
  // PWM bl
  BL_SEL |= BL_PIN;
  TA1CCR0 = BL_FADE_MAX;
  TA1CCTL1 = OUTMOD_6;
  TA1CTL = TASSEL_2 | MC_3;   //Selectionne l'horloge rapide et mode up/down
  
  // interruption sur les boutons
  BTN_IES |= BTN_MENU|BTN_HAUT|BTN_BAS; // high to low transition
  BTN_IE |= BTN_MENU|BTN_HAUT|BTN_BAS;  // interruptions activées
  
  DCF_IE |= DCF_DAT;    // interruption activée
  DCF_IES |= DCF_DAT;   // high to low transition
  
  maj=1;
  
  config =0;
  
  // retro-éclairage
  blMin = 3;
  blMax = BL_FADE_MAX;
  setBL(blMin);
  
  // parametres par défaut
  jour = 31;
  mois = 12;
  annee = 1983;
  jourSem = 1;
  heure.heures = 23;
  heure.minutes = 58;
  delaiSoleil = 40;
  
  //definition des menus
  mPrincipal.titre = strReglage;
  mPrincipal.nbOptions = 5;
  mPrincipal.strOptions = optsMenuPrincipal;
  mPrincipal.ecran = ECRAN_MENU_PRINCIPAL;
  
  mEclairage.titre = strEclairage+1;    //+1 pour supprimer l'espace du début
  mEclairage.nbOptions = 5;
  mEclairage.strOptions = optsMenuEclairage;
  mEclairage.ecran = ECRAN_MENU_ECLAIRAGE;
  
  aMAJ = MAJ_HEURE3 | MAJ_DATE | MAJ_DCF | MAJ_SOLEIL | MAJ_ALARME;
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
    heure.secondes++;
    if(blTimer>0)
      if(--blTimer==0)
      {
        blFade=1;
        blFadeStep = -BL_FADE_STEP;
      }
    if(heure.secondes==60)
    {
      heure.secondes=0;
      heure.minutes++;
      if(heure.minutes==60)
      {
       heure.minutes=0;
       heure.heures++;
       if(heure.heures==24)
       {
        heure.heures=0;
        jour++;
        if(jourSem<6)
          jourSem++;
        else
          jourSem=0;
        if(jour>jourDansMois[mois-1])        
        {
          jour=1;
          mois++;
          if(mois==13)
          {
            mois=1;
            annee++;
          }
        }
        if(ecran == ECRAN_HEURE)
          aMAJ |= MAJ_DATE;
       }
      }
      if(ecran == ECRAN_HEURE)
        aMAJ|= MAJ_HEURE3;
    }
  }
}
