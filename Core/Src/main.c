/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include"fonts.h"
#include"SH1106.h"
#include"bitmap.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t X = 0;
uint16_t Y = 0;
bool G_code = 0;
bool czy_podniesiony_mazak;
uint16_t wysokość_ramki = 14;
uint16_t szerokość_ramki = 100;
enum DeviceState{
	MainMenu, //tryb menu glownego (wybor pomiedzy rysownaiem a kalibracja)
	Calibration, //tryb kalibracji gdzie mazak jedzie na dowolna pozycje i czeka na wypoziomowanie
	FileMenu, //tryb selektora plikow projektowych
	ConfirmFile, //stan w ktorym urzadzenie yta o potwierdzenie pliku
	Drawing //tryb w ktorym wykonywany jest program
};
enum DeviceState Stan;
uint8_t k; //To jest zmienna do wyboru opcji w menu glownym
uint8_t i;  //To zmienna do wyboru plikow
//enum ktory mowi w jakim stanie jest urzadzenie


char *nazwy_plików[] = {
		  "Plik1",
		  "Plik2",
		  "Plik3",
		  "Plik4",
};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

  /* USER CODE BEGIN 1 */

/* FUNKCJA DO USTAWIANIA POZYCJI SERWO */
void SetServoPosition(int16_t angle)
{
	uint16_t pulse = 500 + (angle*11.11);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
    HAL_Delay(500);
}
/* FUNKCJA KTÓRA W ARGUMENCIE MA PODANE, CZY CHCEMY MIEĆ PODNIESIONY MAZAK, CZY NIE
 * PORÓWNUJE TO ZE ZMIENNĄ GLOBALNĄ I USTAWIA MAZAK W ODPOWIEDNIEJ POZYCJI
 */
void mazak(bool pozycja_mazaka){

	if (pozycja_mazaka == czy_podniesiony_mazak){
		return;
	}
	else if(pozycja_mazaka == 1 && czy_podniesiony_mazak == 0){
		SetServoPosition(120);
		czy_podniesiony_mazak = 1;
	}
	else if(pozycja_mazaka == 0 && czy_podniesiony_mazak == 1){
		SetServoPosition(180);
		czy_podniesiony_mazak = 0;
	}
}
/*DODANIE FUNKCJI DO KALIBRACJI, KTÓRA OPUSZCZA MAZAK PRZEZ PEWIEN CZAS, TAK ABY UŻYTKOWNIK
 * MÓGŁNA SPOKOJNIE ZAMONTOWAC MAZAK DOTYKAJACY KARTKI. FUNKCJA NASTEPNIE PODNOSI MAZAK, KTORY JEST
 * JUZ ODPOWIEDNIO WYPOZIOMOWANY
 */
void leveling(){
	mazak(0);
	HAL_Delay(100);
	if(czy_podniesiony_mazak == 0){
		mazak(1);//opuszczenie mazaka
		HAL_Delay(20000);
		//na razie dam czas na wypiziomowanie ale pozniej bedzie tutuaj petla while z której bedzie
		//mozna wyjsc przyciskiem
	}
	mazak(0);//ponowne podniesienie mazaka
}
/* ZAMIANA NA RUCH LINIOWY, KORZYSTAMY Z FUNKCJI S = V*t, GDZIE S TO X LUB Y,
 * NASTĘPNIE KORZYSTAMY Z DANYCH: SILNIK KROKOWY MA : 200KROKÓW/OBR, MICROSTEPPING = 1/2,
 * SKOK ŚRUBY WYNOSI 1.25MM. PARAMETRY SYGNAŁU PWM: PERIOD = 99, PRESCALER = 71,
 * USTAWIAMY WSPÓŁCZYNNIK ARR = 499, TERAZ MAMY WZÓR: fPWM​= fTIM/(PSC+1)×(ARR+1) = 72000000/(71+1)*(499+1)=2000 HZ,
 * NASTĘPNIE (200 KROKÓW/OBR * 2)/1.25 = 320 KROKÓW/MM (GDZIE DWA SĄ OD MICROSTEPPINGU, 1.25 SKOK ŚRUBY,
 * NASTĘPNIE V = 2000/320 = 6.25 mm/s ---> 160mm/ms
 *      fPWM ------^   ​​
 */
uint16_t kierunek_silnikaX(uint16_t x){

	if(x >= X){
		HAL_GPIO_WritePin(DIR_1_GPIO_Port, DIR_1_Pin, GPIO_PIN_RESET);
		x = x - X;
		X = X + x;
	}
	else{
		HAL_GPIO_WritePin(DIR_1_GPIO_Port, DIR_1_Pin, GPIO_PIN_SET);
		x = X - x;
		X = X - x;
	}
	return x;
}
uint16_t kierunek_silnikaY(uint16_t y){

	if(y >= Y){
		HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, GPIO_PIN_SET);
		y = y - Y;
		Y = Y + y;
	}
	else{
		HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, GPIO_PIN_RESET);
		y = Y - y;
		Y = Y - y;
	}
	return y;
}

//bool ruchliniowy(uint16_t x, uint16_t y){
//	uint16_t ARRY = 0;
//	uint16_t ARRX = 0;
//	uint16_t t = 0;
//
//	x = kierunek_silnikaX(x);
//	y = kierunek_silnikaY(y);
//
//	if(x == 0){
//		ARRY = 499;
//		ARRX = 0;
//		t = 160 * y;
//		__HAL_TIM_SET_AUTORELOAD(&htim2, ARRY);
//		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
//	}
//	else if(y == 0){
//		ARRX = 499;
//		ARRY = 0;
//		t = 160 * x;
//		__HAL_TIM_SET_AUTORELOAD(&htim1, ARRX);
//		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
//	}
//	else if(y >= x && y!=0 && x!=0){
//		ARRY = 499;
//		ARRX = (uint16_t)(((float)y / (float)x) * (ARRY + 1) - 1);
//		t = (uint16_t)((float)(8*(ARRX+1))/25) * x;
//		__HAL_TIM_SET_AUTORELOAD(&htim2, ARRY);
//		__HAL_TIM_SET_AUTORELOAD(&htim1, ARRX);
//
//		__HAL_TIM_SET_COUNTER(&htim1, 0);
//		__HAL_TIM_SET_COUNTER(&htim2, 0);
//
//		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
//		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
//	}
//	else{
//		ARRX = 499;
//		ARRY = (uint16_t)(((float)x / (float)y) * (ARRX+1) - 1);
//		t = (uint16_t)((float)(8*(ARRY+1))/25) * y;
//		__HAL_TIM_SET_AUTORELOAD(&htim2, ARRY);
//		__HAL_TIM_SET_AUTORELOAD(&htim1, ARRX);
//
//		__HAL_TIM_SET_COUNTER(&htim1, 0);
//		__HAL_TIM_SET_COUNTER(&htim2, 0);
//
//		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
//		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
//	}
//
////	__HAL_TIM_SET_AUTORELOAD(&htim2, ARRY);
////	__HAL_TIM_SET_AUTORELOAD(&htim1, ARRX);
//
//
////	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // włącz PWM (kroki)
////	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // włącz PWM (kroki)
//
//
//	HAL_Delay(t);
//	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
//	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
//
//return 1;
//
//}
bool ruchliniowy(uint16_t x_target, uint16_t y_target) {

    uint16_t dx = (x_target >= X) ? (x_target - X) : (X - x_target);
    uint16_t dy = (y_target >= Y) ? (y_target - Y) : (Y - y_target);

    if (dx == 0 && dy == 0) return true;

    HAL_GPIO_WritePin(DIR_1_GPIO_Port, DIR_1_Pin, (x_target >= X) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, (y_target >= Y) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    uint16_t ax = 499, ay = 499;
    uint32_t t_ms = 0;

    if (dx >= dy) {
        ax = 499;
        ay = (dy == 0) ? 499 : (uint16_t)(((float)dx / (float)dy) * 500.0f - 1.0f);
        t_ms = (uint32_t)dx * 160;
    } else {
        ay = 499;
        ax = (dx == 0) ? 499 : (uint16_t)(((float)dy / (float)dx) * 500.0f - 1.0f);
        t_ms = (uint32_t)dy * 160;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim1, ax);
    __HAL_TIM_SET_AUTORELOAD(&htim2, ay);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    if (dx > 0) HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    if (dy > 0) HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    HAL_Delay(t_ms);

    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);

    X = x_target;
    Y = y_target;

    HAL_Delay(5);
    return true;
}
/* FUNKCJA USTALAJĄCA POZYCJĘ ZEROWĄ(POCZĄTKOWĄ) */

bool homing(){

	SetServoPosition(180);
	czy_podniesiony_mazak = 0;

	HAL_GPIO_WritePin(DIR_1_GPIO_Port, DIR_1_Pin, GPIO_PIN_SET);

	while(HAL_GPIO_ReadPin(AXIS_X_GPIO_Port, AXIS_X_Pin)){
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // włącz PWM (kroki)
		__HAL_TIM_SET_AUTORELOAD(&htim1, 500 - 1);
	}
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

	HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, GPIO_PIN_RESET);

	while(HAL_GPIO_ReadPin(AXIS_Y_GPIO_Port, AXIS_Y_Pin)){
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // włącz PWM (kroki)
		__HAL_TIM_SET_AUTORELOAD(&htim2, 500 - 1);
	}
	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
	ruchliniowy(5,5);
	X = 0;
	Y = 0;
	return 1;
}

/*
 * KARTA SD
 */
#define MAX_FILES 4

char filenames[MAX_FILES][13];
char ostatnia_linia_pliku[128];
FATFS fs;
DIR dir;
FILINFO fno;
FIL fil;

void list_files(void)
{
    FRESULT res;

    uint8_t i = 0;
    res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        printf("Mount error: %d\n", res);
        return;
    }

    res = f_opendir(&dir, "/");
    if (res != FR_OK) {
        printf("Dir error: %d\n", res);
        return;
    }

    for (;;) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
            break;

        if (fno.fattrib & (AM_SYS | AM_HID | AM_DIR))
        	continue;
        else
            printf("%s (%lu bytes)\n", fno.fname, fno.fsize);
        	snprintf(filenames[i], sizeof(filenames[i]), "%s", fno.fname);
        	i++;
    }

    f_closedir(&dir);
}
void wykonaj_linie(const char *line)
{
    uint16_t temp_X = X;
    uint16_t temp_Y = Y;
    bool temp_Z = czy_podniesiony_mazak;
    int temp_G_val = 0;

    int found_G = 0, found_X = 0, found_Y = 0, found_Z = 0;

    // Parsowanie G
    const char *ptr_G = strchr(line, 'G');
    if (ptr_G != NULL) {
        if (sscanf(ptr_G, "G%d", &temp_G_val) == 1) {
            temp_Z = (temp_G_val == 0) ? 1 : 0;
            found_G = 1;
        }
    }

    // Parsowanie X
    const char *ptr_X = strchr(line, 'X');
    if (ptr_X != NULL) {
        if (sscanf(ptr_X, "X%hu", &temp_X) == 1) {
            found_X = 1;
        }
    }

    // Parsowanie Y
    const char *ptr_Y = strchr(line, 'Y');
    if (ptr_Y != NULL) {
        if (sscanf(ptr_Y, "Y%hu", &temp_Y) == 1) {
            found_Y = 1;
        }
    }

    // Parsowanie Z
    const char *ptr_Z = strchr(line, 'Z');
    if (ptr_Z != NULL) {
        int z_val;
        if (sscanf(ptr_Z, "Z%d", &z_val) == 1) {
            temp_Z = (bool)z_val;
            found_Z = 1;
        }
    }
     if (found_G == 1 || found_Z == 1) {
        if(temp_Z == 1){
            mazak(0);
        } else {
            mazak(1);
        }
    }
    ruchliniowy(temp_X, temp_Y);
    HAL_Delay(10);
}

void linia_po_lini(const char *filename)
{
    FRESULT res;
    char line_buffer[128];
    uint8_t i = 0;
    res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        printf("Mount error: %d\n", res);
        return;
    }
    res = f_opendir(&dir, "/");
    if (res != FR_OK) {
        printf("Dir error: %d\n", res);
        return;
    }
    ostatnia_linia_pliku[0] = '\0';
    res = f_open(&fil, filename, FA_READ);
    if (res != FR_OK) {
        printf("Error: Nie można otworzyć pliku '%s'. Kod: %d\n", filename, res);
        return;
    }

    while (f_gets(line_buffer, sizeof(line_buffer), &fil) != NULL)
    {
        strncpy(ostatnia_linia_pliku, line_buffer, 128 - 1);
        ostatnia_linia_pliku[128 - 1] = '\0';
        wykonaj_linie(ostatnia_linia_pliku);
    }
    if (f_error(&fil)) {
        printf("\nError: Błąd podczas czytania pliku.\n");
    }

    f_close(&fil);
    f_closedir(&dir);
}

/*
 * WYSWIETLACZ
 */
void wyświetl_pluto(){
	  SH1106_DrawBitmap(32,0, pluto_map, 64, 64, 1);
	  SH1106_UpdateScreen();
 }

void wyświetlanie_menu_głównego(uint8_t k){
	  SH1106_Fill(SH1106_COLOR_BLACK);

	  SH1106_GotoXY(47,1);
	  SH1106_Puts("PLUTO", &Font_7x10, SH1106_COLOR_WHITE);

	  if(k == 0){
		  SH1106_DrawRectangle(9,11,52 ,52,SH1106_COLOR_WHITE);
		  SH1106_DrawBitmap(10, 12, olowek_map, 50, 50, 1);
		  SH1106_DrawRectangle(70,11, 52,52,SH1106_COLOR_WHITE);
		  SH1106_DrawBitmap(71, 12, zembatka2_map, 50, 50, 1);
	  }
	  else if(k == 1){
		  SH1106_DrawFilledRectangle(9,11,52 ,52,SH1106_COLOR_WHITE);
		  SH1106_DrawBitmap(10, 12, olowek_map, 50, 50, 0);
		  SH1106_DrawRectangle(70,11, 52,52,SH1106_COLOR_WHITE);
		  SH1106_DrawBitmap(71, 12, zembatka2_map, 50, 50, 1);
	  }
	  else{
		  SH1106_DrawRectangle(9,11,52 ,52,SH1106_COLOR_WHITE);
		  SH1106_DrawBitmap(10, 12, olowek_map, 50, 50, 1);
		  SH1106_DrawFilledRectangle(70,11, 52,52,SH1106_COLOR_WHITE);
		  SH1106_DrawBitmap(71, 12, zembatka2_map, 50, 50, 0);
	  }
	  SH1106_UpdateScreen();

  }

void wyświetlanie_plików(uint8_t i){
	  list_files();
	  SH1106_Fill(SH1106_COLOR_BLACK);
	  for(int j = 0; j < 4; j++ ){
		  if(j == i){
			  SH1106_DrawFilledRectangle(2, 2 + j*16, szerokość_ramki, wysokość_ramki,SH1106_COLOR_WHITE);
			  SH1106_GotoXY(6,5 + j*16);
			  SH1106_Puts(filenames[j], &Font_7x10, SH1106_COLOR_BLACK);
		  }
		  else{
			  SH1106_DrawRectangle(2,2 + j*16, szerokość_ramki, wysokość_ramki,SH1106_COLOR_WHITE);
			  SH1106_GotoXY(6,5 + j*16);
			  SH1106_Puts(filenames[j], &Font_7x10, SH1106_COLOR_WHITE);
		  }
	  }

	  SH1106_DrawLine(117,15,120,10, SH1106_COLOR_WHITE);
	  SH1106_DrawLine(120,10,123,15, SH1106_COLOR_WHITE);

	  SH1106_DrawLine(117,30,120,35, SH1106_COLOR_WHITE);
	  SH1106_DrawLine(120,35,123,30, SH1106_COLOR_WHITE);

	  SH1106_DrawLine(113,55,124,55, SH1106_COLOR_WHITE);
	  SH1106_DrawLine(117,57,124,55, SH1106_COLOR_WHITE);
	  SH1106_DrawLine(117,53,124,55, SH1106_COLOR_WHITE);

	  SH1106_UpdateScreen();
}
uint8_t kordynaty_napisu_pliku(char *napis){
	size_t dlugosc = strlen(napis);
	uint8_t x = (132 - (dlugosc * 7))/2;
	return x;
}
void wyświetlanie_czy_uruchomić_plik(uint8_t i){
	uint8_t x = kordynaty_napisu_pliku(filenames[i]);
	SH1106_Fill(SH1106_COLOR_BLACK);
	SH1106_GotoXY(19,5);
	SH1106_Puts("Wybrano plik: ", &Font_7x10, SH1106_COLOR_WHITE);
	SH1106_GotoXY(x,15);
	SH1106_Puts(filenames[i], &Font_7x10, SH1106_COLOR_WHITE);
	SH1106_GotoXY(19,25);
	SH1106_Puts("Czy uruchomic ", &Font_7x10, SH1106_COLOR_WHITE);
	SH1106_GotoXY(33,35);
	SH1106_Puts("ten plik? ", &Font_7x10, SH1106_COLOR_WHITE);
	SH1106_GotoXY(88,50);
	SH1106_Puts("OK", &Font_7x10, SH1106_COLOR_WHITE);
	SH1106_DrawLine(110,55,124,55, SH1106_COLOR_WHITE);
	SH1106_DrawLine(117,57,124,55, SH1106_COLOR_WHITE);
	SH1106_DrawLine(117,53,124,55, SH1106_COLOR_WHITE);
	SH1106_UpdateScreen();
}


//funkcja do obsługi stanu urządzenia
void ExecuteState(){
	if(Stan == MainMenu){
		wyświetlanie_menu_głównego(k);
		if(HAL_GPIO_ReadPin(SWITCH1_GPIO_Port, SWITCH1_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			if(k==1) k=2;

		}
		if(HAL_GPIO_ReadPin(SWITCH2_GPIO_Port, SWITCH2_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			if(k==2) k=1;
		}
		if(HAL_GPIO_ReadPin(SWITCH3_GPIO_Port, SWITCH3_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			if(k == 1){
				Stan = FileMenu;
			}
			else{
				Stan = Calibration;
			}
		}
	}
	if(Stan == Calibration){
		SH1106_Fill(SH1106_COLOR_BLACK);
		SH1106_DrawRectangle(40,6, 52,52,SH1106_COLOR_WHITE);
		SH1106_DrawBitmap(41, 7, zembatka2_map, 50, 50, 1);
		SH1106_UpdateScreen();
		leveling();
		homing();

		if(HAL_GPIO_ReadPin(SWITCH3_GPIO_Port, SWITCH3_Pin)== GPIO_PIN_RESET){
			HAL_Delay(500);
			Stan = MainMenu;
		}
		Stan = MainMenu;
	}
	if(Stan == FileMenu){
		wyświetlanie_plików(i);
		if(HAL_GPIO_ReadPin(SWITCH1_GPIO_Port, SWITCH1_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			i++;
			if(i==4)i = 0;
		}
		if(HAL_GPIO_ReadPin(SWITCH2_GPIO_Port, SWITCH2_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			i--;
			if(i==255)i = 3;
		}
		if(HAL_GPIO_ReadPin(SWITCH3_GPIO_Port, SWITCH3_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			Stan = ConfirmFile;
		}
	}
	if(Stan == ConfirmFile){
		wyświetlanie_czy_uruchomić_plik(i);
		if(HAL_GPIO_ReadPin(SWITCH1_GPIO_Port, SWITCH1_Pin)==GPIO_PIN_RESET || HAL_GPIO_ReadPin(SWITCH2_GPIO_Port, SWITCH2_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			Stan = FileMenu;
		}
		if(HAL_GPIO_ReadPin(SWITCH3_GPIO_Port, SWITCH3_Pin)==GPIO_PIN_RESET){
			HAL_Delay(500);
			Stan = Drawing;
		}

	}
	if(Stan == Drawing){
		SH1106_Fill(SH1106_COLOR_BLACK);
		SH1106_DrawRectangle(40,6,52 ,52,SH1106_COLOR_WHITE);
		SH1106_DrawBitmap(41, 7, olowek_map, 50, 50, 1);
		SH1106_UpdateScreen();
		linia_po_lini(filenames[i]);
		HAL_Delay(4000);
		Stan = MainMenu;
	}
}


int main(void)
{

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */

  SH1106_Init();
  SH1106_UpdateScreen();

  wyświetl_pluto();
  HAL_Delay(3000);

  homing();
  HAL_Delay(20);

  k = 1;
  i = 0;
  Stan = MainMenu;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  ExecuteState();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
