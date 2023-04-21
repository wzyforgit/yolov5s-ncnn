# Yolov5s 实践记录

作者：南风LI (GitHub: wzyforgit)

本文在 CC BY-ND 4.0 协议下发布，如需转载或引用，请遵循此协议的规范

## 概述

&emsp;&emsp;Yolov5 作为目前工业界使用频率最高的目标检测算法之一，其最火的 U 版更是已经迭代到了 v7.0 版本（[仓库在这里](https://github.com/ultralytics/yolov5)）。我将其作为了假期的实践课题之一，并在这几天展开了初步的研究。

## 训练部分

&emsp;&emsp;为了学到更多有用的东西，我将从最初的训练开始执行本次课题，如本文标题所示，我将选用 Yolov5s 作为学习的目标。

### 架设训练环境

&emsp;&emsp;训练环境没什么好说的，都是老生常谈的问题，只是很多业余爱好者喜欢去在 Windows 下折腾 Anaconda 或者 WSL，这在我看来是非常浪费时间的行为，因为你将面对并不适合进行 AI 开发的 Windows 环境，并在这上面将大量的时间用于环境架设上。

&emsp;&emsp;这里我选用 Debian 11 作为基础系统装入我的移动硬盘内，这样我就可以方便地切换 Windows 和 Linux。随后的事情就简单了：

- 安装 NVIDIA 闭源驱动
- 安装 NVIDIA-Container-Toolkit
- 拉取 Pytorch 2.0 的 Docker 镜像，内部有配置好的 CUDA 和 cuDNN 环境，无需在宿主机上再装一次
- 拉取 U 版的 Yolov5 7.0 源码包
- 拉取 coco 2017 数据集，这里我使用了 Free Download Manager 来加速下载

&emsp;&emsp;至此就已配置好了基础的训练环境，接下来就是折磨人的训练过程了。

### 执行训练过程

&emsp;&emsp;首先最基本的便是给 Yolov5 的源码工程找个舒服的地方存着，然后将其挂载进 Docker 环境中，并执行 `pip3 install -U -r requirements.txt` 安装缺失的 Python 包，接下来就是漫长的启动训练过程了。

#### 数据格式问题

&emsp;&emsp;要启动训练并不容易，我们要面对 coco 数据集的格式和 Yolov5 的输入格式不匹配的问题。这里有两种处理方式：

- 自行编写脚本将 coco 的数据格式转为 Yolov5 的形式
- 使用源码工程里的配置项自动下载转换好的格式

&emsp;&emsp;这里我选择了第二种方式处理这个问题，由于直接使用源码工程去下载数据集会导致它触发单线程下载，然后下很久都不会下好，因此我们需要做一点处理：

- 打开源码工程下的 data/coco.yaml 文件，注释掉里面的 `download(urls, dir=dir / 'images', threads=3)`，这将阻止它下载庞大的 coco 数据集本体

- 修改 `data/coco.yaml` 文件的 `path` 一栏，将其修改为存放数据集的位置，当然，这里需要使用相对路径进行描述，毕竟我们不可能将整个系统都挂载进 Docker

- 执行训练命令 `python3 train.py --data coco.yaml --cfg yolov5s.yaml --weights '' --batch-size 1 --device 0 --epochs 1 --img 640`，这里它就会去尝试下载转换好的数据格式到指定目录

- 等待下载完成后，将我们用 Free Download Manager 下载好的数据解压并拷贝至对应的目录中，然后再注释掉最后的下载命令 `download(urls, dir=dir.parent)`。此时再启动训练它可能会报告在 `/Dataset` 下找不到文件，这里我们打一个软连接过去就搞定了。

&emsp;&emsp;当训练可以如期进行后，我们需要提交一次 Docker 环境以备后续使用。

#### Docker 内存问题

&emsp;&emsp;训练的时候可能会遭遇报告共享内存不足的问题，此时需要在启动 Docker 的时候加上参数 `--shm-size 24G`，当然我这里是因为我的机器有 32 GB 内存，所以写了个 24 G 上去。实测下来整个训练过程会占用 7 ~ 10 GB 内存，可以据此酌情分配内存空间。

#### 显存与 batch size 问题

&emsp;&emsp;由于我的设备比较龊，显卡仅有 8 GB 显存，因此 batch size 仅能开到 32，此时的 Docker 显存占用为 6 GB 左右，加上我再开个网页和代码编辑器，显存占用就来到了 7 GB 之多，基本算满载了。

&emsp;&emsp;不过由于作者优秀的代码，我的显卡计算负载全程保持在了 90% 以上，也基本上是榨干了它全部的性能。

#### Pytorch 2.0 加速问题

&emsp;&emsp;众所周知，Pytorch 的版本号从 1.x 滚到 2.x 最大的改变就是引入了模型预编译，这样可以让它的训练速度提升一个台阶。但不幸的是，Yolov5 7.0 并没有支持这一特性（毕竟发布的时候 Pytorch 2.0 还在内测），如果强行打开会导致后续的训练代码报很多奇奇怪怪的错误，目前 issue 区也已经有人报告了这个问题，我等白嫖党就静待作者支持吧。

#### 训练数据保存

&emsp;&emsp;每次启动训练，代码都会在源码工程的 runs/train 目录下创建一个新的文件夹用于保存最佳和最后的权重数据，而当预定的 epoch 跑完后，它还会生成一些和训练数据相关的图片用于训练者参考。

### 导出模型

&emsp;&emsp;这部分较为简单，U 版的代码支持导出非常多的模型格式，根据需要执行命令 `python3 export.py --weights runs/train/exp/weights/best.pt --include torchscript` 即可。这里需要注意指定权重文件的位置，代码会自动识别模型的版本，无需手动指定。最后导出的模型格式不同，可能需要额外安装其它的依赖包（详情见 `requirements.txt`），由于我这里导出的是 Pytorch 自家的 torchscript 格式，因此不需要准备其它的依赖项，直接就能使用。

## 部署部分

&emsp;&emsp;这里我直接选择了 NCNN 作为部署框架，它最大的优势在于其强大的跨平台能力和简单易用的特性，官方的仓库地址是 https://github.com/tencent/ncnn/

### 架设部署开发环境

#### C++ 工程依赖项

&emsp;&emsp;部署侧开发环境需要我们能快速而直观地看到可视化的计算结果，这里如本仓库所示，我选择了 Qt 作为 GUI 开发工具和图片的预处理工具。而对于 NCNN 而言，我选择了一个讨巧的方式：由于我的基础系统是 Debian 11，而目前 Debian 阵营中仅有 Deepin 将其集成进了仓库（hahaha，其实也是我本人打包进去的），因此我直接拉取 [Deepin 的代码](https://github.com/deepin-community/ncnn) 并在本地构建了一个 NCNN 的 deb 包，安装完成后再使用 cmake 去引用它。

&emsp;&emsp;其实要封装成标准的 SDK，可能用 [OpenCV-Mobile](https://github.com/nihui/opencv-mobile) 这类精简后的工具包作为图片的预处理工具更好，但我这里偷懒就直接 Qt 一波流了 =。=

#### 模型转换工具

&emsp;&emsp;由于我的部署工具选择了 NCNN，而对接的模型格式又是 torchscript，因此这里我理所当然地选择了 PNNX 作为模型转换工具。PNNX 的源码挂在 NCNN 仓库的 `tools/pnnx` 目录下，不会参与主工程的构建。如有需要，有两种使用方式可供选择：

- 参照文档自行拉取 libtorch 的相关库进行构建
- 前往 [PNNX 的发布仓库](https://github.com/pnnx/pnnx) 拉取已编译好的文件直接使用

&emsp;&emsp;这里我采用的方案是直接拉取 [最新已编译好的文件](https://github.com/pnnx/pnnx/releases/download/20230420/pnnx-20230420-ubuntu.zip)，然后对着已导出的 torchscript 文件执行 `./pnnx best.torchscript inputshape=[1,3,640,640]`，即可获取能够直接使用的 best.ncnn.param 和 best.ncnn.bin 文件。

### 了解 Yolov5 的数据格式

&emsp;&emsp;在编写代码前，还有必要了解一下这个模型的数据输入输出格式：

- 通过检查 Yolo 的源码工程的 `detect.py` 文件，可以看到输入是一个 RGB888 格式的图片，在输入模型之前需要将三个通道的数值全部除以 255 以归一化到 0 ~ 1 之间

- 模型的输出是一个 shape 为 [1,1,25200,85] 的 tensor，意为存在 25200 组数据，每组数据有 85 个

&emsp;&emsp;需要注意的是，这 85 个数据也分为两个部分：

- 前 5 个数据分别是目标矩形框的 x_center,y_center,width,height,prob，即矩形框的中心坐标、宽高、矩形框成立的概率

- 后 80 个数据和分类有关，它代表了该矩形框内的东西是对应标签的概率，由于 coco 数据集里面有 80 个不同的标签，因此这里的输出是 80 个概率数据

&emsp;&emsp;在了解它的输出后，处理代码就变得很简单了。

### 编写处理代码

&emsp;&emsp;由于强大的 PNNX 和 NCNN 现在已经完整支持 Yolov5 的后处理模块，加上 U 版仓库移除了恼人的 Focus 模块，这使得我们可以无痛使用 PNNX 转换出来的模型（见本工程的 `yolov5s.cpp` 文件）：

- 前面的预处理部分没啥好说的，先 padding 到 640 * 640，然后对齐 RGB888 再全部乘以 1.0 / 255.0 即可

- 在经过模型取得数据后，需要先去除矩形框和分类数据中无效的部分，具体做法是先判断矩形框的 prob 是否高于对应的阈值，然后再判断后续的分类概率中的最大值是否高于对应的阈值，两者均高于阈值即表示本检测结果有效，可以进入下一轮筛选，同时分类概率的最大值对应的标签即为本检测结果代表的标签

- 完成数据初筛后，需要再执行一次 NMS 算法，即非极大抑制算法。它的处理流程非常简单：将每一个分类标签下的矩形框单独拎出来成立一个组，每个组内从概率最大的开始和其它的框进行 PK，如果这个框和后续的框的 IOU 值大于阈值，则低概率的框将会直接被剔除，剔除完一轮后，如果还有除了概率最大框以外的框存活，则下一个框将会成为新的概率最大框（因为它通过了和刚才那个框的筛选），然后和剩下的框继续 PK，直至没有剩下框

- IOU 意为两个矩形框的交集面积和它们的并集面积的比值，这个值越大，即表示它们重合的部分越大，那么就越有必要剔除那个概率较低的矩形框

- 最后把经过 NMS 算法筛选的框集合起来，送入 GUI 模块进行显示即可

&emsp;&emsp;至此便完成了完整的 Yolov5s 从训练到部署的全套流程。

## 参考资料

[coco数据集标注说明 1](https://zhuanlan.zhihu.com/p/101984674)

[coco数据集标注说明 2](https://www.cnblogs.com/Meumax/p/12021913.html)

[coco数据集API说明](https://blog.csdn.net/qq_41709370/article/details/108471072)

[yolov5启动训练](https://www.cxyzjd.com/article/sinat_28442665/108813518)

[yolov5导出模型](https://zhuanlan.zhihu.com/p/587224902)

## FAQ

Q: 看网上教程好多都是在捣鼓 anchor 框，为啥你这里没捣鼓这些

A: 因为目前 PNNX 和 NCNN 支持导出 Yolov5 的后处理了，anchor 框的解码部分已经集成进了模型内部。但是昨天和社区内部大佬讨论的结果是，这样虽然代码能少写很多，但速度并不是最快的，因为我们每次都需要解码 25200 个框！但在解码之前我们是可以拿到这些框的 prob 数据的，如果根据 prob 数据预先干掉一大堆无用的框，最终仅解码有解码价值的框，那么处理速度将会提升一个台阶

这部分的教程见 https://github.com/Tencent/ncnn/discussions/4541

Q: 为啥最终 NCNN 的模型体积相较于 torchscript 小了一半？

A: 因为 PNNX 默认打开了 fp16 存储，使得原来以 fp32 存储的模型缩小了

Q: 为啥这个工程挂了一个很严厉的 AGPL-3.0-or-later ？

A: emmmmmm，因为 U 版的 Yolov5 7.0 就是这个协议 orz，用了他们的代码训练模型，自然就全变成这个了
