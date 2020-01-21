在golang中只有三种引用类型它们分别是切片slice、字典map、管道channel
/***/
数组
Go 语言中的数组是一种 值类型
var arr1 = new([5]int)	arr1 的类型是 *[5]int
var arr2 [5]int			arr2 的类型是 [5]int
这样的结果就是当把一个数组赋值给另一个时，需要在做一次数组内存的拷贝操作
切片（slice）是对数组一个连续片段的引用
优点 因为切片是引用，所以它们不需要使用额外的内存并且比使用数组更有效率，所以在 Go 代码中 切片比数组更常用
make 	slice {array指向数组指针, len切片长度, cap切片预留内存}
append 	slice 需要扩容则翻倍, 源slice数据拷贝到新slice指向的数组
copy 	slice 源切片数据逐个拷贝到目的切片数组, 拷贝数量去两个切片长度最小值, copy不发生扩容


/***/
map
map 是 引用类型 的： 内存用 make 方法来分配。
不要使用 new，永远用 make 来构造 map
注意 如果你错误的使用 new() 分配了一个引用对象，你会获得一个空引用的指针，相当于声明了一个未初始化的变量并且取了它的地址

/***/
new和make
在编译期间的 类型检查 阶段，Go 语言其实就将代表 make 关键字的 OMAKE 节点根据参数类型的不同转换成了 OMAKESLICE、OMAKEMAP 和 OMAKECHAN 三种不同类型的节点，这些节点最终也会调用不同的运行时函数来初始化数据结构

结构与方法
当一个匿名类型被内嵌在结构体中时，匿名类型的可见方法也同样被内嵌，这在效果上等同于外层类型 继承 了这些方法：将父类型放在子类型中来实现亚型

/***/
string和[]byte
string {str指向字符串首地址, len字符串长度}
[]byte {array指向字符串数组的指针, len切片长度, cap切片预留内存}

[]byte转string	
1. 申请string内存空间
2. 构建string{str, len}
3. 拷贝数据

string转[]byte
1. 申请切片内存空间
2. 将string拷贝到切片

字符串拼接
编译时将所有字符子串放入切片, 第一次遍历所有切片获取长度, 分配新字符串内存, 第二次遍历所有切片把字符串拷贝到新字符串

string不允许修改
字符串只有字符串指针和长度, 不包含真正的分配内存空间, 这样做轻量方便传递且不用担心内存拷贝
字符串指针指向字符串字面量存储在只读片段而非堆栈, 因此string不可修改, 相同string常量不重复存储


/***/
闭包
闭包=函数+引用环境
Go 语言支持匿名函数，可作为闭包。匿名函数是一个"内联"语句或表达式。匿名函数的优越性在于可以直接使用函数内的变量，不必申明。
当我们不希望给函数起名字的时候，可以使用匿名函数，例如：func(x, y int) int { return x + y }。
关键字 defer （详见第 6.4 节）经常配合匿名函数使用，它可以用于改变函数的命名返回值。
匿名函数还可以配合 go 关键字来作为 goroutine 使用（详见第 14 章和第 16.9 节）。
匿名函数同样被称之为闭包（函数式语言的术语）：它们被允许调用定义在其它环境下的变量。闭包可使得某个函数捕捉到一些外部状态，例如：函数被创建时的状态。另一种表示方式为：一个闭包继承了函数所声明时的作用域。这种状态（作用域内的变量）都被共享到闭包的环境中，因此这些变量可以在闭包中被操作，直到被销毁，详见第 6.9 节中的示例。闭包经常被用作包装函数：它们会预先定义好 1 个或多个参数以用于包装，详见下一节中的示例。另一个不错的应用就是使用闭包来完成更加简洁的错误检查（详见第 16.10.2 节）。




/***/
封装 继承 多态
package main
 
import "fmt"
 
type Creature struct {
  Name string
  Real bool
}
 
func Dump(c*Creature) {
  fmt.Printf("Name: '%s', Real: %t\n", c.Name, c.Real)
}
 
func (c Creature) Dump() {
  fmt.Printf("Name: '%s', Real: %t\n", c.Name, c.Real)
}
 
type FlyingCreature struct {
  Creature
  WingSpan int
}
 
func (fc FlyingCreature) Dump() {
  fmt.Printf("Name: '%s', Real: %t, WingSpan: %d\n",
    fc.Name,
    fc.Real,
    fc.WingSpan)
}
 
type Unicorn struct {
  Creature
}
 
type Dragon struct {
  FlyingCreature
}
 
type Pterodactyl struct {
  FlyingCreature
}
 
func NewPterodactyl(wingSpan int) *Pterodactyl {
  pet := &Pterodactyl{
    FlyingCreature{
      Creature{
        "Pterodactyl",
        true,
      },
      wingSpan,
    },
  }
  return pet
}
 
type Dumper interface {
  Dump()
}
 
type Door struct {
  Thickness int
  Color     string
}
 
func (d Door) Dump() {
  fmt.Printf("Door => Thickness: %d, Color: %s", d.Thickness, d.Color)
}
 
func main() {
  creature := &Creature{
    "some creature",
    false,
  }
 
  uni := Unicorn{
    Creature{
      "Unicorn",
      false,
    },
  }
 
  pet1 := &Pterodactyl{
    FlyingCreature{
      Creature{
        "Pterodactyl",
        true,
      },
      5,
    },
  }
 
  pet2 := NewPterodactyl(8)
 
  door := &Door{3, "red"}
 
  Dump(creature)
  creature.Dump()
  uni.Dump()
  pet1.Dump()
  pet2.Dump()
 
  creatures := []Creature{
    *creature,
    uni.Creature,
    pet1.Creature,
    pet2.Creature}
  fmt.Println("Dump() through Creature embedded type")
  for _, creature := range creatures {
    creature.Dump()
  }
 
  dumpers := []Dumper{creature, uni, pet1, pet2, door}
  fmt.Println("Dump() through Dumper interface")
  for _, dumper := range dumpers {
    dumper.Dump()
  }
}

Name: 'some creature', Real: false
Name: 'some creature', Real: false
Name: 'Unicorn', Real: false
Name: 'Pterodactyl', Real: true, WingSpan: 5
Name: 'Pterodactyl', Real: true, WingSpan: 8
Dump() through Creature embedded type
Name: 'some creature', Real: false
Name: 'Unicorn', Real: false
Name: 'Pterodactyl', Real: true
Name: 'Pterodactyl', Real: true
Dump() through Dumper interface
Name: 'some creature', Real: false
Name: 'Unicorn', Real: false
Name: 'Pterodactyl', Real: true, WingSpan: 5
Name: 'Pterodactyl', Real: true, WingSpan: 8
Door => Thickness: 3, Color: red








/***/
defer

清理释放资源
多个defer执行顺序
被deferred函数的参数在defer时确定

type _defer struct {
	siz 	int32		//目标函数参数长度
	started bool
	sp 		uintptr 	//调用deferproc时栈指针sp
	pc 		uintptr 	//调用deferproc时调用方程序计数器pc
	fn 		*funcval	//dfer关键字中传入的函数
	_panic 	*_panic
	link 	*_defer
}

type p struct {
	deferpool [5][]*_defer
}

type g struct {
	_defer *_defer
}


编译器将defer处理成两个函数调用, deferproc定义一个延迟调用对象, 然后在函数exit结束前插入deferreturn的调用
运行时, 每一个defer关键字都会被转换成deferproc, 在这个函数中我们会为defer创建一个新的_defer结构体并设置它的 fn pc 和 sp 参数
deferproc
	*调用newdefer初始化一个新的_defer结构体
		*调用deferclass获取参数长度对齐后的缓存等级
		*未超出缓存大小则从deferpool提取
		*超过缓存大小则调用mallocgc分配_defer + siz的大小
		*设置d.siz = siz
		*将d保存到g._defer链表
	*设置d.fn = fn
	*设置d.pc = pc
	*设置d.sp = sp
	*参数复制到dfer对象后的内存空间
	
	
deferreturn
	*获取g._defer链表的dfer延迟对象
	*如果dfer延迟对象为空则return
	*对比dfer.sp避免调用其他栈帧的延迟函数
	*调整g._defer链表
	*释放_defer对象, 放回缓存
	*调用jmpdefer执行延迟函数
	
jmpdefer是汇编函数
jmpdefer
	*跳转到dfer所在的代码执行并且在执行结束后跳转回deferreturn函数
	*defereturn函数会多次判断当前Goroutine中是否有剩余的_defer 结构直到所有的_defer都执行完毕, 这时当前函数才会返回