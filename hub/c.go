package main

/*
#include "glue.h"
#cgo LDFLAGS: -L. -lbh_expr -lstdc++ -lm -lboost_system
*/
import "C"
import (
	"log"
)

type Fun struct {
	fun C.Fun
}

func test_C() {
	log.Println(C.test())
}

func MkFun(s string) Fun {
	return Fun{fun: C.MkFun(C.CString(s))}
}
