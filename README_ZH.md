# everyfile
为了解决在linux下，通过文件名快速检索全局文件，支持文件名模糊查询，以及文件的实时检索。 <br>
## 安装与运行
内核版本推荐5.7以上,编译安装内核模块需要依赖linux kernel的头文件，不同发行版本安装命令略有不同。 <br>
需要添加atomic动态库，gcc需要支持atomic，在较低版本的gcc中该功能仅提供接口但未实现，能通过编译但运行时会出现异常。 <br>
需要添加sqlite动态库。 <br>
<pre>
cd everyfile/kernelmod
make
sudo insmod vfs_kprobe.ko
cd everyfile
make clean && make
</pre>
## 程序设计说明
