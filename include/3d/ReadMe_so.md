## 接口说明

| CsvCreatePoint3DParam |  |
| --------------------- | ---- |
| string calibXml | 设备标定出来的XML文件存放绝对路径 |
| string modelPathFolder | 接口内部创建的模型文件存放目录 |
| CSV_DataFormatType type | 输出点云的类型（暂时未用）注：现在出两种格式：1、浮点的点云数据；2、ushort的深度图，第一行用于解成点云的参数组； |
| 返回值 | 无 |


| bool CsvSetCreatePoint3DParam(const CsvCreatePoint3DParam &param) | 读取XML同时启动初始化 |
| --------------------- | ---- |
| param | CsvCreatePoint3DParam参数类的引用输入 |
| bool返回值 | true：设置成功；false：设置失败 |


| bool CsvGetCreatePoint3DParam(CsvCreatePoint3DParam &param) | 读出参数类 |
| --------------------- | ---- |
| param	| CsvCreatePoint3DParam参数类的引用输出 |
| bool返回值 | true：读出成功；false：读出失败 |


| bool CsvCreateLUT(const CsvCreatePoint3DParam &param) | 生成算法流程需要的模型。需要第一个运行且运行一次 |
| --------------------- | ---- |
| param	| CsvCreatePoint3DParam参数类的引用输入 |
| bool返回值 | true：创建模型成功；false：创建模型失败 |


| bool CsvCreatePoint3D(const std::vector<std::vector<CsvImageSimple>>& inImages, CsvImageSimple &depthImage, std::vector<float> *point3D) | 传入结构光图像组，传出深度图和点云 |
| --------------------- | ---- |
| inImages	| CsvImageSimple类型的图像组。左右眼图像分两组存放，每组13张图片（亮暗图，7个格雷码图，4个相移图） |
| depthImage | 传出的ushort型深度图，以左眼坐标系为参考坐标系 |
| point3D | 传出的float型点云图。如果为空指针，则无输出 |
| bool返回值 | true：计算点云成功；false：计算点云失败 |


### 小工具

| bool CsvC1RangeImageTo3DCloudXYZ(cv::Mat& inRangeImage16, std::vector<float>& out3DXYZ) | 生成的自解析深度图转换为点云图 |
| --------------------- | ---- |
| inRangeImage16 | OpenCV格式的深度图 |
| out3DXYZ | Float3格式的点云图 |
| bool返回值 | true：成功；false：失败 |





## 版本说明


| V3.1.0.221026B |
| ------------- |
| 1、深度图中加入标定信息，可使用深度图独立转成点云，无需依赖其它信息； |
| 2、提供一个深度图转点云的函数：CsvC1RangeImageTo3DCloudXYZ |
| 3、修改了部分导致程序奔溃的情况，如标定文件错误等。 |


| V3.0.0.20221020 |
| ------------- |
| 1、计算点云和深度图的功能都放入GPU内执行 |



