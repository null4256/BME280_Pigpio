# BME280_Pigpio
BME280をpigpiodで動かすためのライブラリです。
使用方法
コンストラクタ　BME280(int pi,int i2c_num)
初期化するために必要です。
piにはpigpiodのpigpio_start()で取得したpiを、i2c_numにはI2Cのナンバー(gpio2,gpio3使用なら1番)を選択してください
失敗時にはプログラムが強制終了します

セット関数　Setting(Device_Info device)
BME280に測定情報を書き込みます。
BME280の測定を始めるのに必須です。
使用手順は
1. BME280::Device_Info型を作成
2. 必要な情報を構造体に書き込む
3. 引数にその構造体を入れる
です
失敗時にはstring型が、成功時にはvoidが返ってきます。

Read_Data(bool pressure,bool humidity)
BME280の取得データを読みます。
この関数を実行するとbme280.datasの変数が更新され、気圧(Pa)、温度(℃)、湿度(%)が取得できます。
pressure,humidityにはデータが欲しいときにはtrue,いらないなら何も書かなくてよいです。
データを温度は100で、気圧は256で、湿度は1024で割れば測定値が出ます。
