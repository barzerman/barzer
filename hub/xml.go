package main

import (
	"encoding/xml"
)

type BarzerQuery struct {
	XMLName xml.Name `xml:"query"`
	Data    string   `xml:",chardata"`
	Ret     string   `xml:"ret,attr"`
	Beni    string   `xml:"beni,attr"`
	Uid     uint32   `xml:"u,attr"`
}

type BarzerUniverse struct {
	Uid    uint32  `xml:"uid,attr"`
	Name   string  `xml:"name,attr"`
	Weight float64 `xml:"weight,attr"`
}

type BarzerInstance struct {
	Host   string  `xml:"host,attr"`
	Port   uint16  `xml:"port,attr"`
	Weight float64 `xml:"weight,attr"`
}

type BarzerConfig struct {
	XMLName   xml.Name         `xml:"config"`
	Universes []BarzerUniverse `xml:"multiverse>universe"`
	Instances []BarzerInstance `xml:"instances>instance"`
}
