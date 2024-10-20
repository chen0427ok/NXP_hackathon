# 梅竹黑客松 - 智耕未來

## 團隊簡介

- 團隊名稱：希希不嘻嘻
- 成員姓名：李松曄、陳致希、陳苗原、廖年弘、曾立綸
- 競賽議題：恩智浦半導體 - 智慧應用改善氣候變遷

## 專案簡介

- **構想：**<br>
現今全球氣候變遷和農業生產面臨的挑戰中，如何提升農業效率並提高作物產量，成為各國農業產業關注的焦點。農作物的生長環境如溫度、土壤濕度等，將直接影響其生長速度、產量和品質。然而，傳統的農業管理方法存在監測範圍有限和操作不精確等問題。因此，我們決定運用環境感測器和 NXP 企業之 i.MX RT1060EVK 開發版來智能監控農業環境條件，以幫助提升農業生產效率。

- **用途 / 功能：**<br>
透過 i.MX RT1060EVK 開發板，連接溫溼度、土壤濕度感測器獲取數據，將數據輸入已導入開發板之 AI 模型，預測當前環境條件對於玉米生長之產量，並以視覺化網頁呈現，協助使用者改善環境條件。

- **目標客群 & 使用情境：**
    - 農業工作者：使用此工具對作物進行狀況進行監測，依據 AI 建議改善作物產量
    - 科學研究人員：使用此工具監控環境數據，並連結其他數據分析農作物與環境的關聯

- **操作方式：**
    1. 將環境感測器放置適當的位置以監測環境數據
    2. 將環境感測器連接至 i.MX RT1060EVK 開發板，並且連接至電腦開啟程式
    3. 開啟網頁查看視覺化數據，並且依據 AI 建議改善當前環境條件

## 使用資源

### 硬體資源

- i.MX RT1060EVK 開發版
- IW416 Wi-Fi 模組
- LCD Panel
- DHT11 溫濕度傳感器模組
- YL-69 土壤濕度感測器

### 軟體資源

- MCUXpresso IDE
- GUI Guider
- Google Colab
- [Corn Crop Growth 資料集](https://www.kaggle.com/datasets/miguelh65/corn-crop-growth)
- RandomForestRegressor 機器學習演算法

## 技術細節

### 讀取感測器數據

- 在 DHT11 進行通訊時特定的時間協議非常重要，這裡使用 `delayms` 和 `delayus` 完成毫秒和微秒級的延遲。
```c
static void delayms(int ms)
{
    volatile uint32_t i = 0;
    for (i = 0; i < ms*70000; ++i)
    {
    	__NOP(); // 創建延遲
    }
}

static void delayus(int us)
{
    volatile uint32_t i = 0;
    for (i = 0; i < us*70; ++i)
    {
    	__NOP(); // 創建延遲
    }
}
```

- `read_data_bit` 函數用來從 DHT11 感測器讀取單個位元。DHT11 在傳送數據時，會透過高低電壓的持續時間來表示位元。此函數首先等待電壓變高，接著延遲 35 微秒，然後檢查信號線是否仍然為高電壓來判定這一位是 1 還是 0。
```c
uint8_t read_data_bit(void)
{
    uint8_t bit = 0;
    while(GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 0); // 等待電壓變高
    delayus(35);  // 等待 35 微秒
    if(GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 1)  // 如果電壓仍為高則為該位元為 1
    {
        bit = 1;
    }
    while(GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 1); // 等待電壓變低
    return bit;
}
```

- 宣告兩個 `gpio_pin_config_t` 型態變數，分別用來配置 DHT11 的腳位為數位輸出模式和數位輸入模式。在初始化 DHT11 時，必須先設置為輸出模式來發送啟動訊號，隨後再切換為輸入模式來接收數據。
```c
gpio_pin_config_t USER_DHT_out_config = {
    .direction = kGPIO_DigitalOutput,
    .outputLogic = 1U,
    .interruptMode = kGPIO_NoIntmode
};

gpio_pin_config_t USER_DHT_in_config = {
    .direction = kGPIO_DigitalInput,
    .outputLogic = 0U,
    .interruptMode = kGPIO_NoIntmode
};
```

- 首先把 DHT11 的腳位設定為輸出模式，然後拉低電壓持續 18 毫秒，這是啟動訊號。接著拉高電壓並延遲 40 微秒，然後把腳位切換為輸入模式，準備接收來自 DHT11 的數據。再來從 DHT11 讀取 5 個數據，分別存入 `data` 陣列。每個數據位是 8 位元，透過 `read_data_bit` 函數一個一個讀取。最後輸出 `data[0]`（濕度）和 `data[2]`（溫度），以便在 Debug Console 上查看讀取的結果。
```c
GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_out_config);
delayms(2000);
GPIO_PinWrite(DHT11_GPIO,DHT11_GPIO_PIN,0);
delayms(18);
GPIO_PinWrite(DHT11_GPIO,DHT11_GPIO_PIN,1);
delayus(40);
GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_in_config);
delayus(180);

uint8_t data[5] = {0};
for(int i = 0; i < 5; i++)
{
    for(int j = 7; j >= 0; j--)
    {
        data[i] |= (read_data_bit() << j);
    }
}

PRINTF("%d, %d\r\n", data[0], data[2]);
```

### 使用 Wi-Fi 連接至網頁視覺化設計

將網頁所使用到的 UI 介面，透過程式轉為 C 程式碼後存放於開發版中，並透過 IW416 Wi-Fi 模組連網後開啟網頁，即可在相同網域下透過手機連線至開發板。

- 啟動 Wi-Fi 掃描並提示輸入網路名稱密碼
```c
PRINTF("[i] Starting Wi-Fi scan...\r\n");
char *ssids = WPL_Scan(); // 進行 Wi-Fi 網路的掃描，並返回可用的 SSID

PRINTF("[i] Please enter the SSID of the network you want to connect to: ");
fflush(stdout);  // 強制刷新，確保輸出即時顯示
char user_ssid[WPL_WIFI_SSID_LENGTH] = {0};
scanf("%s", user_ssid);  // 等待使用者輸入 SSID 並將其儲存

PRINTF("\n[i] Please enter the password for the network: ");
fflush(stdout);  // 強制刷新，確保輸出即時顯示
char user_password[WPL_WIFI_PASSWORD_LENGTH] = {0};
scanf("%s", user_password);  // 等待使用者輸入密碼並將其儲存
```
- 嘗試連接至指定的 Wi-Fi 網路
```c
PRINTF("\n[i] Attempting to connect to SSID: %s\r\n", user_ssid);
PRINTF("\n[i] Attempting to connect to password: %s\r\n", user_password);
fflush(stdout);  // 強制刷新，確保輸出即時顯示
result = WPL_AddNetworkWithSecurity(user_ssid, user_password, WIFI_NETWORK_LABEL, WPL_SECURITY_WPA2); // 將 Wi-Fi 網路添加到開發板的網路列表，並設置 WPA2 安全協定
```
- 檢查是否成功加入網路
```c
if (result == WPLRET_SUCCESS) // 檢查Wi-Fi網路是否成功添加
{
    result = WPL_Join(WIFI_NETWORK_LABEL);
    if (result == WPLRET_SUCCESS)
    {
        g_BoardState.wifiState = WIFI_STATE_CLIENT;
        g_BoardState.connected = true;
        PRINTF("[i] Successfully connected to SSID: %s\r\n", user_ssid);
        char ip[32];
        WPL_GetIP(ip, 1); // 獲取當前連接網路的 IP 地址
        PRINTF(" Now join that network on your device and connect to this IP: %s\r\n", ip);
    }
    else
    {
        PRINTF("[!] Failed to join SSID: %s\r\n", user_ssid);
    }
}
else
{
    PRINTF("[!] Failed to add network SSID: %s\r\n", user_ssid);
}
```
- 啟動 Web 伺服器
```c
if (xTaskCreate(http_srv_task, "http_srv_task", HTTPD_STACKSIZE, NULL, HTTPD_PRIORITY, NULL) != pdPASS) // 啟動 HTTP 伺服器的任務
{
    PRINTF("[!] HTTPD Task creation failed.");
    while (1)
        __BKPT(0);
}
```

### GUI Guider

初始化和控制 LCD 螢幕顯示是利用 LVGL（Light and Versatile Graphics Library）來管理 UI 和顯示更新。以下程式用以負責初始化 LVGL、顯示器、輸入設備以及用戶界面，並進行任務處理。

- 在 LVGL 庫啟用日誌功能（`LV_USE_LOG`）的情況下，註冊一個日誌輸出的回調函數 `print_cb`，並且註冊閒置時間回調函數
```c
#if LV_USE_LOG
    lv_log_register_print_cb(print_cb);
#endif

lv_timer_register_get_idle_cb(get_idle_time_cb); // 用來取得系統的閒置時間
lv_timer_register_reset_idle_cb(reset_idle_time_cb); // 重置閒置計時器
```
- 進行初始化設定
```c
lv_port_pre_init(); // 用來進行 LVGL 的前置初始化
lv_init(); // 初始化 LVGL 的核心系統
lv_port_disp_init(); // 初始化顯示驅動
lv_port_indev_init(); // 初始化輸入設備

s_lvgl_initialized = true;
setup_ui(&guider_ui); // 設定 UI 的布局和元素
events_init(&guider_ui); // 初始化 UI 元素的事件處理
custom_init(&guider_ui); // 根據應用需求進行自定義初始化

#if LV_USE_VIDEO
    Video_InitPXP(); // 初始化 PXP
#endif
```

- 無限迴圈處理任務
```c
for (;;)
{
    lv_task_handler(); // 處理所有的 LVGL 相關任務
    vTaskDelay(1); // 延遲 1 個作業系統時脈
    vTaskSuspend(NULL); // 暫停當前任務，直到再次被喚醒
}
```

### 隨機森林模型

- 使用 `pandas` 讀取資料集並且進行特徵選取，並且以 8:2 比例分割訓練集與測試集
```python
import pandas as pd

file_path = '/content/crop_growth_dataset.csv'
data = pd.read_csv(file_path) # 讀取資料集

X = data[['Temperature', 'Humidity', 'Soil_Moisture']]  # 環境特徵
y = data['Growth']  # 目標變量

# 分割資料集
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
```

- 使用 `RandomForestRegressor` 訓練模型
```python
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error, r2_score

# 訓練模型
rf_model = RandomForestRegressor(n_estimators=120, random_state=42)
rf_model.fit(X_train, y_train)

# 在測試集上進行預測
y_pred = rf_model.predict(X_test)

# 評估模型表現
mse = mean_squared_error(y_test, y_pred)
r2 = r2_score(y_test, y_pred)

print(f"均方誤差 (MSE): {mse}")
print(f"R² 決定係數: {r2}")
```

- 轉為 C 程式碼，將 AI 模型導入至開發版
```python
import m2cgen as m2c

c_code = m2c.export_to_c(rf_model)

with open("random_forest_model.c", "w") as file:
    file.write(c_code)
```

- 將 C 程式碼中的 `score` 函式改寫為輸出百分格式，分數計算公式為「30 + 70 * (預測高度 - 資料集中最小生長高度) / (資料集中最大生長高度 - 資料集中最小生長高度)」，並且將分數最大值設定為 99.9 分。
```c
double score(double *input) {
    // ... 原始程式碼
    // predict_height 為預測生長高度
    double min_height = 36.97; // 資料集中最小生長高度
    double max_height = 136.31; // 資料集中最大生長高度
    double predict_score = 30 + 70 * (predict_height - min_height) / (max_height - min_height); // 基本分 30 分用以避免分數落差過大
    double output_score = min(predict_score, 99.9);
    return output_score;
}
```

- 新增 `suggest` 函式，對當前環境條件進行判斷，給出調整建議。分為 6 種情境進行判斷，分別為「當前溫度降低 5 %」、「當前溫度提升 5 %」、「當前濕度降低 5 %」、「當前濕度提升 5 %」、「當前土壤濕度降低 5 %」與「當前土壤濕度提升 5 %」。函式輸出以下值分別代表不同涵義：
  - 輸出 `0`：當前分數為 80 分以上不需調整，或 6 種調整情境皆無法提高分數
  - 輸出 `1`：建議降低溫度
  - 輸出 `2`：建議提升溫度
  - 輸出 `3`：建議降低溫度
  - 輸出 `4`：建議提升濕度
  - 輸出 `5`：建議降低土壤濕度
  - 輸出 `6`：建議提升土壤濕度
```c
int suggest(double *input)
{
    double x = input[0], y = input[1], z = input[2];

    double temp_range = 35 - 10;
    double humidity_range = 90 - 30;
    double soilmoisture_range = 40 - 10;
    double ratio = 0.05;

    double suggestion_features[6][3] = {
        {x - temp_range * ratio, y, z},
        {x + temp_range * ratio, y, z},
        {x, y - humidity_range * ratio, z},
        {x, y + humidity_range * ratio, z},
        {x, y, z - soilmoisture_range * ratio},
        {x, y, z + soilmoisture_range * ratio}};

    int suggestion = 0;
    double suggestion_score = 0;
    double orig_score = score(input);

    if (orig_score >= 80)
    {
        return 0;
    }

    for (int i = 1; i <= 6; i++)
    {
        double cur_score = score(suggestion_features[i - 1]);
        if (cur_score > suggestion_score)
        {
            suggestion_score = cur_score;
            suggestion = i;
        }
    }

    return (suggestion_score > orig_score ? suggestion : 0);
}
```

## 未來展望

### 改變傳統農業模式

- **增加多元感測器提升效率：**<br>
除了溫濕度外，作物產量還受土壤酸鹼、肥沃度、光照、蟲害等因素影響。透過增加不同類型的感測器來全面監測作物生長環境，並結合鏡頭與 AI，自動偵測植物病徵或蟲害問題，能迅速做出反應與處理，突破傳統農業的人力限制。

- **結合物聯網應用於智慧管理：**<br>
利用物聯網將各類感測器數據即時傳輸至中央系統，進行遠程監控與控制。使用者可在行動裝置上即時查看作物生長狀況，並對水資源、肥料及農藥用量進行精準管理，從而減少資源浪費，降低人力成本，達到農作物產量最大化的目標。

### 火星農業的未來藍圖

- **發展封閉式農業系統：**<br>
火星上的農業需要在封閉的溫室或地下基地內進行，這些系統將完全依賴人工設施來控制氣候、土壤條件、水分和光照。我們未來能將智慧農業系統整合到封閉式自給自足系統中，利用物聯網技術自動化調整環境變數，對植物的健康狀況進行實時監控。
- **農業資源再利用：**<br>
火星上所有資源都極其寶貴，封閉循環系統至關重要。我們可以將封閉循環系統加入智慧農業系統中，將動植物之代謝產物循環利用，為植物提供永續生長環境。

## 開發過程

### 遇到的困難
- **DHT11 感測器的訊號讀取問題：**<br>
與一般的感測器不同，DHT11 為單線雙向通訊的感測器，且讀取資料的方式對時脈十分嚴格，首先須對該腳位輸出低電位 18ms、高電位 40us，之後轉為輸入，等待 DHT11 的等待訊號結束才開始讀取。DHT11 的資料以透過高電位的長短決定為 `1` 或 `0`，70us 為 `1`，26us 為 `0`，因此我們需要精度能達到微秒的 timer 來讀取數據，最開始嘗試的是 FreeRTOS 的 `Taskdelay`，但若要處理微秒尺度則需改變系統的 tick，會導致後續 task 效率低落因此淘汰；再來是 `SystemTick`，`SystemTick` 能確實讀取也較為簡單，但在後續結合 GUIguider 或 Wi-Fi 卻發現 `Task` 跟 `SystemTick` 會互相影響而出錯。
    - **解決方法：**<br>
    最後採用的是最原始的方式 `NOP()`，`NOP()` 所具有的問題則是實際的 ClockCycle 與計算理想值不同，需要利用其他方式測量並經過調整才能符合需求。

- **將 AI 模型導入至開發板：**<br>
原先我們的計畫為使用 TensorFlow 訓練神經網路模型進行預測，然後將已訓練模型使用 TensorFlow Lite Micro 應用於開發板上。然而目前 TensorFlow Lite Micro 在網路上的資源有限，在導入過程中遇到許多不明錯誤且難以解決。
    - **解決方法：**<br>
    改用隨機森林演算法訓練 AI 模型，並且使用 `m2cgen` 套件將模型轉換為 C 程式碼。

## 成果展示
- [Google 簡報]()
