# VAR-SOM-MX8M-PLUS 200-pin J1 pinout (base config)

| Pin | Signal | Bank | Type | Alternates |
|----:|--------|------|------|------------|
| 1 | ENET_TX_CTL | ETH | bidirectional | NC |
| 2 | GND | PWR | power_in |  |
| 3 | ENET_TD3 | ETH | bidirectional | ETH0_MDI_A_P |
| 4 | ENET_RD0 | ETH | bidirectional | ETH0_MDI_C_P |
| 5 | ENET_TD2 | ETH | bidirectional | ETH0_MDI_A_M |
| 6 | ENET_RD1 | ETH | bidirectional | ETH0_MDI_C_M |
| 7 | GND | PWR | power_in |  |
| 8 | GND | PWR | power_in |  |
| 9 | ENET_TD1 | ETH | bidirectional | ETH0_MDI_B_P |
| 10 | ENET_RD2 | ETH | bidirectional | ETH0_MDI_D_P |
| 11 | ENET_TD0 | ETH | bidirectional | ETH0_MDI_B_M |
| 12 | ENET_RD3 | ETH | bidirectional | ETH0_MDI_D_M |
| 13 | GND | PWR | power_in |  |
| 14 | GND | PWR | power_in |  |
| 15 | ENET_RX_CTL | ETH | bidirectional | ETH0_LED_ACT |
| 16 | ENET_RXC | ETH | bidirectional | ETH0_LED_LINK_10_100_1000 |
| 17 | SPDIF_EXT_CLK | AUD | bidirectional |  |
| 18 | SAI3_TXD | AUD | bidirectional | DMIC_CLK |
| 19 | GND | PWR | power_in |  |
| 20 | SAI3_MCLK | AUD | bidirectional | DMIC_DATA |
| 21 | SAI2_RXD0 | AUD | bidirectional |  |
| 22 | SAI2_RXC | AUD | bidirectional |  |
| 23 | SAI2_RXFS | AUD | bidirectional |  |
| 24 | SAI2_TXFS | AUD | bidirectional |  |
| 25 | SAI2_TXC | AUD | bidirectional |  |
| 26 | SAI2_TXD0 | AUD | bidirectional |  |
| 27 | GND | PWR | power_in |  |
| 28 | GND | PWR | power_in |  |
| 29 | GPIO1_IO15 | GPIO | bidirectional |  |
| 30 | ENET_MDIO | ETH | bidirectional |  |
| 31 | NAND_DATA01 | SDMMC | bidirectional | NC, CONN_SD1_DATA1 |
| 32 | VCC_SOM | PWR | power_in |  |
| 33 | NAND_DATA02 | SDMMC | bidirectional | NC, CONN_SD1_DATA2 |
| 34 | VCC_SOM | PWR | power_in |  |
| 35 | NAND_DATA03 | SDMMC | bidirectional | NC, CONN_SD1_DATA3 |
| 36 | VDD_ENET0_1P8_3P3_IN | PWR | power_in | NC |
| 37 | GND | PWR | power_in |  |
| 38 | VDD_ENET1_1P8_3P3_IN | PWR | power_in |  |
| 39 | ECSPI2_SS0 | COMMS | bidirectional |  |
| 40 | GPIO1_IO00 | GPIO | bidirectional | SAI1_MCLK |
| 41 | ECSPI2_MISO | COMMS | bidirectional |  |
| 42 | BOOT_SEL | PWR | bidirectional |  |
| 43 | ECSPI2_SCLK | COMMS | bidirectional |  |
| 44 | UART3_RXD | COMMS | bidirectional |  |
| 45 | ECSPI2_MOSI | COMMS | bidirectional |  |
| 46 | UART3_TXD | COMMS | bidirectional |  |
| 47 | GND | PWR | power_in |  |
| 48 | SAI2_MCLK | AUD | bidirectional |  |
| 49 | SOM_3V3_PER | PWR | power_out |  |
| 50 | ECSPI1_MISO | COMMS | bidirectional |  |
| 51 | ECSPI1_SS0 | COMMS | bidirectional |  |
| 52 | ECSPI1_MOSI | COMMS | bidirectional |  |
| 53 | ECSPI1_SCLK | COMMS | bidirectional |  |
| 54 | SAI1_RXD7 | AUD | bidirectional |  |
| 55 | SAI1_TXD3 | AUD | bidirectional |  |
| 56 | SAI1_TXD2 | AUD | bidirectional |  |
| 57 | SAI1_TXC | AUD | bidirectional |  |
| 58 | ENET_TXC | ETH | bidirectional | NC |
| 59 | GND | PWR | power_in |  |
| 60 | SD2_CLK | SDMMC | bidirectional |  |
| 61 | SD2_DATA2 | SDMMC | bidirectional |  |
| 62 | SD2_DATA0 | SDMMC | bidirectional |  |
| 63 | SD2_DATA1 | SDMMC | bidirectional |  |
| 64 | SD2_CMD | SDMMC | bidirectional |  |
| 65 | SD2_DATA3 | SDMMC | bidirectional |  |
| 66 | GND | PWR | power_in |  |
| 67 | GND | PWR | power_in |  |
| 68 | SPDIF_TX | AUD | bidirectional |  |
| 69 | GPIO1_IO11 | GPIO | bidirectional |  |
| 70 | GPIO1_IO13 | GPIO | bidirectional | SAI1_RXD0, IND_RST_BT |
| 71 | SAI1_RXD6 | AUD | bidirectional |  |
| 72 | GPIO1_IO05 | GPIO | bidirectional | SAI1_RXD2, NC |
| 73 | SAI1_TXD0 | AUD | bidirectional |  |
| 74 | ENET_MDC | ETH | bidirectional |  |
| 75 | GPIO1_IO01 | GPIO | bidirectional | SAI1_RXD3, WCI-2_OUT |
| 76 | GND | PWR | power_in |  |
| 77 | GPIO1_IO08 | GPIO | bidirectional | SAI1_TXD6, NC |
| 78 | GND | PWR | power_in |  |
| 79 | NAND_DQS | SDMMC | bidirectional |  |
| 80 | GPIO1_IO14 | GPIO | bidirectional |  |
| 81 | SAI1_RXD5 | AUD | bidirectional |  |
| 82 | GPIO1_IO07 | GPIO | bidirectional | SAI1_RXFS, IND_RST_WL |
| 83 | UART2_RXD | COMMS | bidirectional |  |
| 84 | NAND_DATA00 | SDMMC | bidirectional | CONN_SD1_DATA0 |
| 85 | UART2_TXD | COMMS | bidirectional |  |
| 86 | GPIO1_IO06 | GPIO | bidirectional | SAI1_RXC, NC |
| 87 | I2C3_SDA | COMMS | bidirectional |  |
| 88 | I2C3_SCL | COMMS | bidirectional |  |
| 89 | GND | PWR | power_in |  |
| 90 | I2C4_SDA | COMMS | bidirectional |  |
| 91 | USB1_RX_N | USBPCIE | bidirectional |  |
| 92 | I2C4_SCL | COMMS | bidirectional |  |
| 93 | USB1_RX_P | USBPCIE | bidirectional |  |
| 94 | USB1_ID | USBPCIE | bidirectional |  |
| 95 | GND | PWR | power_in |  |
| 96 | SAI1_TXD5 | AUD | bidirectional |  |
| 97 | USB1_TX_P | USBPCIE | bidirectional |  |
| 98 | SYS_nRST_3V3 | PWR | bidirectional |  |
| 99 | USB1_TX_N | USBPCIE | bidirectional |  |
| 100 | PCIE1_REF_CLK_N | USBPCIE | bidirectional |  |
| 101 | GND | PWR | power_in |  |
| 102 | PCIE1_REF_CLK_P | USBPCIE | bidirectional |  |
| 103 | VCC_SOM | PWR | power_in |  |
| 104 | USB2_VBUS | PWR | power_out |  |
| 105 | VCC_SOM | PWR | power_in |  |
| 106 | USB1_VBUS | PWR | power_out |  |
| 107 | VCC_SOM | PWR | power_in |  |
| 108 | USB2_D_N | USBPCIE | bidirectional |  |
| 109 | VCC_SOM | PWR | power_in |  |
| 110 | USB2_D_P | USBPCIE | bidirectional |  |
| 111 | VCC_SOM | PWR | power_in |  |
| 112 | GND | PWR | power_in |  |
| 113 | SAI1_TXD4 | AUD | bidirectional |  |
| 114 | USB1_D_N | USBPCIE | bidirectional |  |
| 115 | UART4_RXD | COMMS | bidirectional |  |
| 116 | USB1_D_P | USBPCIE | bidirectional |  |
| 117 | GPIO1_IO03 | GPIO | bidirectional | SAI1_RXD1, WCI-2_SIN |
| 118 | GND | PWR | power_in |  |
| 119 | MIPI_CSI1_D0_P | DISP | bidirectional |  |
| 120 | SAI1_TXFS | AUD | bidirectional |  |
| 121 | MIPI_CSI1_D0_N | DISP | bidirectional |  |
| 122 | SAI1_RXD4 | AUD | bidirectional |  |
| 123 | MIPI_CSI1_D1_N | DISP | bidirectional |  |
| 124 | UART1_TXD | COMMS | bidirectional |  |
| 125 | MIPI_CSI1_D1_P | DISP | bidirectional |  |
| 126 | GND | PWR | power_in |  |
| 127 | MIPI_CSI1_D2_P | DISP | bidirectional |  |
| 128 | PCIE1_TX_N | USBPCIE | bidirectional |  |
| 129 | MIPI_CSI1_D2_N | DISP | bidirectional |  |
| 130 | PCIE1_TX_P | USBPCIE | bidirectional |  |
| 131 | MIPI_CSI1_D3_N | DISP | bidirectional |  |
| 132 | GND | PWR | power_in |  |
| 133 | MIPI_CSI1_D3_P | DISP | bidirectional |  |
| 134 | PCIE1_RX_P | USBPCIE | bidirectional |  |
| 135 | MIPI_CSI1_CLK_P | DISP | bidirectional |  |
| 136 | PCIE1_RX_N | USBPCIE | bidirectional |  |
| 137 | MIPI_CSI1_CLK_N | DISP | bidirectional |  |
| 138 | GND | PWR | power_in |  |
| 139 | GND | PWR | power_in |  |
| 140 | USB2_RX_P | USBPCIE | bidirectional |  |
| 141 | USB2_TX_N | USBPCIE | bidirectional |  |
| 142 | USB2_RX_N | USBPCIE | bidirectional |  |
| 143 | USB2_TX_P | USBPCIE | bidirectional |  |
| 144 | GND | PWR | power_in |  |
| 145 | EARC_N_HPD | DISP | bidirectional | NAND_ALE, CONN_SD1_CLK |
| 146 | HDMI_TX1_P | DISP | bidirectional | MIPI_CSI2_D1_P |
| 147 | EARC_P_UTIL | DISP | bidirectional | NAND_CE0_B, CONN_SD1_CMD |
| 148 | HDMI_TX1_N | DISP | bidirectional | MIPI_CSI2_D1_N |
| 149 | GND | PWR | power_in |  |
| 150 | HDMI_TXC_N | DISP | bidirectional | MIPI_CSI2_CLK_N |
| 151 | HDMI_TX2_P | DISP | bidirectional | MIPI_CSI2_D2_P |
| 152 | HDMI_TXC_P | DISP | bidirectional | MIPI_CSI2_CLK_P |
| 153 | HDMI_TX2_N | DISP | bidirectional | MIPI_CSI2_D2_N |
| 154 | HDMI_HPD | DISP | bidirectional | MIPI_CSI2_D3_P |
| 155 | HDMI_TX0_P | DISP | bidirectional | MIPI_CSI2_D0_P |
| 156 | HDMI_CEC | DISP | bidirectional | MIPI_CSI2_D3_N |
| 157 | HDMI_TX0_N | DISP | bidirectional | MIPI_CSI2_D0_N |
| 158 | GND | PWR | power_in |  |
| 159 | GND | PWR | power_in |  |
| 160 | LVDS0_D1_N | DISP | bidirectional | MIPI_DSI1_D1_N |
| 161 | LVDS0_D0_N | DISP | bidirectional | MIPI_DSI1_D0_N |
| 162 | LVDS0_D1_P | DISP | bidirectional | MIPI_DSI1_D1_P |
| 163 | LVDS0_D0_P | DISP | bidirectional | MIPI_DSI1_D0_P |
| 164 | LVDS0_D2_N | DISP | bidirectional | MIPI_DSI1_D2_N |
| 165 | LVDS0_D3_N | DISP | bidirectional | MIPI_DSI1_D3_N |
| 166 | LVDS0_D2_P | DISP | bidirectional | MIPI_DSI1_D2_P |
| 167 | LVDS0_D3_P | DISP | bidirectional | MIPI_DSI1_D3_P |
| 168 | LVDS0_CLK_N | DISP | bidirectional | MIPI_DSI1_CLK_N |
| 169 | GND | PWR | power_in |  |
| 170 | LVDS0_CLK_P | DISP | bidirectional | MIPI_DSI1_CLK_P |
| 171 | UART4_TXD | COMMS | bidirectional |  |
| 172 | GND | PWR | power_in |  |
| 173 | GPIO1_IO09 | GPIO | bidirectional | SAI1_TXD7, IND_RST_15.4 |
| 174 | HDMI_DDC_SCL | DISP | bidirectional |  |
| 175 | UART1_RXD | COMMS | bidirectional |  |
| 176 | HDMI_DDC_SDA | DISP | bidirectional |  |
| 177 | SAI1_TXD1 | AUD | bidirectional |  |
| 178 | GND | PWR | power_in |  |
| 179 | GND | PWR | power_in |  |
| 180 | LVDS1_CLK_N | DISP | bidirectional |  |
| 181 | LVDS1_D3_P | DISP | bidirectional |  |
| 182 | LVDS1_CLK_P | DISP | bidirectional |  |
| 183 | LVDS1_D3_N | DISP | bidirectional |  |
| 184 | LVDS1_D0_N | DISP | bidirectional |  |
| 185 | GND | PWR | power_in |  |
| 186 | LVDS1_D0_P | DISP | bidirectional |  |
| 187 | I2C1_SDA | COMMS | bidirectional | TS_X- |
| 188 | LVDS1_D1_N | DISP | bidirectional |  |
| 189 | I2C1_SCL | COMMS | bidirectional | TS_X+ |
| 190 | LVDS1_D1_P | DISP | bidirectional |  |
| 191 | I2C2_SDA | COMMS | bidirectional | SAI2_MCLK, TS_Y+ |
| 192 | LVDS1_D2_N | DISP | bidirectional |  |
| 193 | I2C2_SCL | COMMS | bidirectional | TS_Y- |
| 194 | LVDS1_D2_P | DISP | bidirectional |  |
| 195 | AGND | PWR | power_in |  |
| 196 | SAI3_RXFS | AUD | bidirectional | HPOUTFB |
| 197 | SAI3_RXC | AUD | bidirectional | LINEIN1_LP |
| 198 | SAI3_RXD | AUD | bidirectional | HPLOUT |
| 199 | SAI3_TXFS | AUD | bidirectional | LINEIN1_RP |
| 200 | SAI3_TXC | AUD | bidirectional | HPROUT |
