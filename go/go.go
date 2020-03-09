type Data struct {
  FileSize int32  `json:"size"`
  FileName string `json:"file_name"`
}

type Test struct {
  FileIndex int  `json:"index"`
  Data      Data `json:"data"`
}

//Temp ..
type Temp map[string]interface{}

var test map[string]Temp
test = make(map[string]Temp)

func testFunc() {
  test["1"] = Temp{
    "size": int(637843),
    "name": "test",
  }

  for k, v := range test {
    index := k
    fmt.Println(index)

    temp, ok := v["size"]
    if ok {
      filesize := temp.(int)
      fmt.Println(filesize)
    }

    temp, ok = v["name"]
    if ok {
      fileName := temp.(string)
      fmt.Println(fileName)
    }
  }
}
