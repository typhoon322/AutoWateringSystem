#pragma once

// BLE 外设，供安卓 App 替代 Web 家庭页。
// 广播名 Langua。状态分包通知，命令为短 ASCII 行。

void ble_link_begin();
void ble_link_loop();
