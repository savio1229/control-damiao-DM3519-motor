# control-damiao-DM3519-motor
use arduino control damiao DM3519 motor

用 Arduino + USB Host Shield ，
經由 USB‑CAN 轉接器，用 速度模式 控制達妙 DM‑S3519 馬達，
使能 / 失能並以指定 RPM 轉動。


一、通訊與控制模式概念

速度模式 CAN 協議
達妙說明書定義，速度模式下的 CAN 幀是： 
CAN ID ： 0x200 + ID
資料：D0~D3 = v_des（速度給定，float，小端，單位 rad/s）
D4~D7 不使用
也就是：

要讓 CAN ID = 1 的電機以某速度運轉：
發 CAN 幀 ID = 0x200 + 1 = 0x201，D0~D3 放該速度的浮點數(rad/s)，小端序。 

這份程式正是用同樣規則：速度命令時 motor_id = 0x200 + slaveId，D0~D3 放 rpm → rad/s 的浮點數 bytes。


二、整體架構
硬體結構
Arduino (例如 UNO) + USB Host Shield 2.0
USB Host Shield 接一顆達妙官方或相容的 USB‑CAN 轉接器
USB‑CAN → CAN 線 → DM‑S3519/DM3520‑1EC 驅動器 → 馬達
Arduino 在 USB 上扮演「Host」（原本是 PC 的角色），
USB‑CAN 則是「CDC 虛擬串口裝置」，在 Arduino 看起來像一個高波特率串列埠。
