#!/bin/sh

insmod led_driver.ko &
insmod beep_driver.ko &
insmod jdq_driver.ko &
insmod sg90_driver.ko &
insmod dht11_driver.ko &
insmod sr501_driver.ko &
insmod ap3216c_driver.ko &
insmod ov2640_camera_driver.ko &
