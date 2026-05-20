/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
    int16_t temperature;    // E.g. 255 -> 25.5 C
    uint16_t humidity;      // E.g. 605 -> 60.5 %
    uint32_t pressure;      // 101325 -> 1013.25 hPa (Sensor measures in Pascal)
} Weather_Data_t;


typedef struct {            // BMP280 kalibrációs adatait tároló struktúra
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_Calib_t;

BMP280_Calib_t bmp280_calib; // Globális kalibrációs változó
 
// Behozzuk az UART kezelőt
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1; // Behozzuk a CubeMX által generált I2C-t

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define AHT20_ADDRESS  (0x38 << 1)   // The addresses must be shifted to the left by one
#define BMP280_ADDRESS (0x77 << 1)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for WeatherTask */
osThreadId_t WeatherTaskHandle;
const osThreadAttr_t WeatherTask_attributes = {
  .name = "WeatherTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for weatherQueue */
osMessageQueueId_t weatherQueueHandle;
const osMessageQueueAttr_t weatherQueue_attributes = {
  .name = "weatherQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

uint8_t AHT20_Init(void);
uint8_t AHT20_Read(int16_t *Temperature, uint16_t *Humidity);
uint8_t BMP280_Init(void);
uint8_t BMP280_ReadPressure(uint32_t *Pressure);

/* USER CODE END FunctionPrototypes */

void StartWeatherTask(void *argument);
void StartDisplayTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of weatherQueue */
  weatherQueueHandle = osMessageQueueNew (8, sizeof(Weather_Data_t), &weatherQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of WeatherTask */
  WeatherTaskHandle = osThreadNew(StartWeatherTask, NULL, &WeatherTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartWeatherTask */
/**
  * @brief  Function implementing the WeatherTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartWeatherTask */
void StartWeatherTask(void *argument)
{
  /* USER CODE BEGIN StartWeatherTask */
    Weather_Data_t sensorData;

    // 1. Szenzor ébresztése a ciklus ELŐTT (csak egyszer fut le)
    AHT20_Init();
    BMP280_Init(); // BMP280 elindítása

    /* Infinite loop */
    for(;;)
    {
        // 2. Mérés elindítása
        AHT20_Read(&sensorData.temperature, &sensorData.humidity);
        BMP280_ReadPressure(&sensorData.pressure);
        
        // Bedobjuk a csomagot a Queue-ba
        osMessageQueuePut(weatherQueueHandle, &sensorData, 0, osWaitForever);
        
        // AHT20-at nem szabad túl sűrűn kérdezni, a 3 másodperc tökéletes
        osDelay(3000); 
    }
  /* USER CODE END StartWeatherTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  Weather_Data_t received;
  char buf[100];
  

  /* Infinite loop */
  for(;;)
  {
    // Várunk a Queue-ra. Amíg üres, ez a task "alszik" és nem pazarol CPU-t
    if (osMessageQueueGet(weatherQueueHandle, &received, NULL, osWaitForever) == osOK) {
        
        // Kiszámoljuk a tizedesjegyeket az AHT20-as függvényünkhöz igazítva
        int temp_int = received.temperature / 10;
        int temp_dec = received.temperature % 10;
        
        int hum_int = received.humidity / 10;
        int hum_dec = received.humidity % 10;

        // Negatív hőmérsékletnél a modulo (%) negatív számot adna, ezt korrigáljuk a kijelzésnél
        if (temp_dec < 0) temp_dec = -temp_dec;
        
        // Kiszámoljuk a tizedesjegyeket a BMP280 -hoz
        int press_int = received.pressure / 100;
        int press_dec = received.pressure % 100;

        // Szöveg formázása
      int len = sprintf(buf, "AHT20 -> Temp: %d.%d C | Hum: %d.%d %% | Press: %d.%d hPa\r\n", 
                  temp_int, temp_dec, hum_int, hum_dec, press_int, press_dec);
        
        // Küldés a PC-nek
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, 100);
    }
  }
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/*
 *  --- Reading sensor data ------------------------------------------
 */

uint8_t AHT20_Init(void) {
    // 1. Várunk a bekapcsolás után (az AHT20-nak kell 100-150ms az ébredéshez)
    osDelay(150);
    
    // 2. Szoftveres újraindítás (Reset), hogy tiszta lappal induljunk
    uint8_t reset_cmd = 0xBA;
    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, &reset_cmd, 1, 100);
    osDelay(20);
    
    // 3. KÉNSZERÍTETT KALIBRÁCIÓ (Nem nézünk státuszt, azonnal küldjük!)
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, init_cmd, 3, 100);
    osDelay(50); // Várunk, amíg a kalibráció lefut a chipben

    //uint8_t check_cmd = {0x71}; // Állapot lekérdező parancs
    //uint8_t status;
    
    // 1. Várunk egy kicsit (100ms -ot) a bekapcsolás után (szüksége van rá a chipnek)
    //osDelay(100);
    
    // 2. Lekérjük a státuszt
    //HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, &check_cmd, 1, 100);
    //HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, &status, 1, 100);
    
    // 3. Ellenőrizzük, hogy kalibrálva van-e (a 3. bitnek 1-nek kell lennie)
    //if ((status & 0x08) == 0x00) {
        // Ha nincs kalibrálva, elküldjük az inicializáló parancsot
      //  uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
      //  HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, init_cmd, 3, 100);
      //  osDelay(10);
    //}
    
    return 1;
}

/*
 *  --- Reading sensor data ------------------------------------------
 * */

uint8_t AHT20_Read(int16_t *Temperature, uint16_t *Humidity) {
    uint8_t read_cmd[] = {0xAC, 0x33, 0x00}; // Mérés indítása parancs
    uint8_t data[6];                         // Ide érkezik a 6 bájt adat
    uint32_t raw_humidity = 0;
    uint32_t raw_temperature = 0;

    // 1. Megkérjük a szenzort, hogy mérjen
    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, read_cmd, 3, 100);
    
    // 2. Várunk 80 ms-ot, amíg a szenzor elvégzi a mérést (FreeRTOS delay!)
    osDelay(100);

    // 3. Kiolvassuk a 6 bájtot
    HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, data, 6, 100);


    // 4. Ellenőrizzük a státuszt (az első bájt legfelső bitje (0x80) jelzi, ha még foglalt)
    if ((data[0] & 0x80) == 0x00) {
        
        // AHT20 adat-összerakás (a bitek furcsán, félig eltolva érkeznek)
        //raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((uint32_t)data[3] >> 4);
        //raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5];

        // Egész számú átszámítás (tizedesjegy megtartásával, float nélkül!)
        // A képlet alapján: Hum = (raw / 2^20) * 100
        // Hogy megmaradjon egy tizedesjegy, megszorozzuk 1000-rel a 100 helyett
        //*Humidity = (uint16_t)(((uint64_t)raw_humidity * 1000) >> 20); // Pl: 605 -> 60.5%
         
        // Temp = (raw / 2^20) * 200 - 50. Egy tizedesjegyhez: * 2000, és a végén - 500
        //*Temperature = (int16_t)(((uint64_t)raw_temperature * 2000) >> 20) - 500;  // Pl: 255 -> 25.5°C

         // 1. PÁRATARTALOM (20 bit) pontos maszkolása és összefűzése
        raw_humidity = (((uint32_t)data[1]) << 12) | 
                       (((uint32_t)data[2]) << 4)  | 
                       ((((uint32_t)data[3]) >> 4) & 0x0F);
    
        // 2. HŐMÉRSÉKLET (20 bit) pontos maszkolása och összefűzése
        raw_temperature = ((((uint32_t)data[3]) & 0x0F) << 16) | 
                          (((uint32_t)data[4]) << 8)  | 
                          ((uint32_t)data[5]); 
        
          
        
        // 3. ÁTSZÁMÍTÁS 64 bites szorzással a túlcsordulás ellen
        *Humidity = (uint16_t)(((uint64_t)raw_humidity * 1000) >> 20); 
        *Temperature = (int16_t)((((uint64_t)raw_temperature * 2000) >> 20) - 500); 


        return 1; // Sikeres mérés
    }
    
    return 0; // A szenzor még foglalt volt
}

/*--- BMP280 sensor --------------------------------------------------------------------
 *
 * */

uint8_t BMP280_Init(void) {
    uint8_t calib_data[24];
    uint8_t reset_cmd = 0xB6;  // Szoftveres reset parancs
    uint8_t config_cmd = 0x3F; // Normal mode, Temp x1, Pres x16 (sima változó!)

    // 1. Kényszerített újraindítás (Reset) a tiszta induláshoz
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDRESS, 0xE0, I2C_MEMADD_SIZE_8BIT, &reset_cmd, 1, 100);
    osDelay(20); // Várunk, amíg a chip újraindul

    // 2. Olvassuk ki a 24 bájtnyi gyári kalibrációs adatot a 0x88-as regisztertől kezdve
    if (HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDRESS, 0x88, I2C_MEMADD_SIZE_8BIT, calib_data, 24, 100) != HAL_OK) {
        return 0; // Hiba
    }

    // 3. Másoljuk be a bájtokat a struktúrába
    bmp280_calib.dig_T1 = (calib_data[1] << 8) | calib_data[0];
    bmp280_calib.dig_T2 = (calib_data[3] << 8) | calib_data[2];
    bmp280_calib.dig_T3 = (calib_data[5] << 8) | calib_data[4];
    bmp280_calib.dig_P1 = (calib_data[7] << 8) | calib_data[6];
    bmp280_calib.dig_P2 = (calib_data[9] << 8) | calib_data[8];
    bmp280_calib.dig_P3 = (calib_data[11] << 8) | calib_data[10];
    bmp280_calib.dig_P4 = (calib_data[13] << 8) | calib_data[12];
    bmp280_calib.dig_P5 = (calib_data[15] << 8) | calib_data[14];
    bmp280_calib.dig_P6 = (calib_data[17] << 8) | calib_data[16];
    bmp280_calib.dig_P7 = (calib_data[19] << 8) | calib_data[18];
    bmp280_calib.dig_P8 = (calib_data[21] << 8) | calib_data[20];
    bmp280_calib.dig_P9 = (calib_data[23] << 8) | calib_data[22];

    // 4. Kapcsoljuk be a szenzort (Beírjuk a 0x3F-et a 0xF4-es ctrl_meas regiszterbe)
    // Most már a &config_cmd pontosan a 0x3F értékű változóra mutat!
    if (HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDRESS, 0xF4, I2C_MEMADD_SIZE_8BIT, &config_cmd, 1, 100) != HAL_OK) {
        return 0; // Hiba a bekapcsolásnál
    }

    return 1;
}

uint8_t BMP280_ReadPressure(uint32_t *Pressure) {
    uint8_t raw_data[3];
    int32_t adc_P;
    int32_t var1, var2;
    uint32_t p;

    // 1. Olvassuk ki a 3 bájt nyers nyomásadatot (0xF7, 0xF8, 0xF9 regiszterek)
    if (HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDRESS, 0xF7, I2C_MEMADD_SIZE_8BIT, raw_data, 3, 100) != HAL_OK) {
        return 0;
    }

    // Nyers 20 bites adat összefűzése
    adc_P = (int32_t)(((uint32_t)raw_data[0] << 12) | ((uint32_t)raw_data[1] << 4) | ((uint32_t)raw_data[2] >> 4));

    // 2. Gyári Bosch kompenzációs képlet (Csak egész számokkal!)
    // Mivel a hőmérsékletet az AHT20-szal mérjük, a t_fine értéket fix középértékre állítjuk a képletben
    int32_t t_fine = 100000; // Átlagos belső hőmérsékleti együttható

    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_calib.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)bmp280_calib.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)bmp280_calib.dig_P4) << 16);
    var1 = (((bmp280_calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)bmp280_calib.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)bmp280_calib.dig_P1))) >> 15;

    if (var1 == 0) {
        return 0; // Elkerüljük a 0-val való osztást
    }

    p = (((uint32_t)(((int32_t)1048576) - adc_P)) - (var2 >> 12)) * 3125;
    if (p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }

    var1 = (((int32_t)bmp280_calib.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp280_calib.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp280_calib.dig_P7) >> 4));

    // A 'p' érték Pascalban van (pl: 101325 Pa = 1013.25 hPa)
    *Pressure = p;
    return 1;
}



/* USER CODE END Application */

