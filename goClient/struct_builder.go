package main

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
	"uav_client/src/common"
)

type StringSplitError struct {
	StatusCode int
	Err        error
}

func (r *StringSplitError) Error() string {
	return fmt.Sprintf("status %d: err %v", r.StatusCode, r.Err)
}

func buildTelemetryRequest(i *common.TelemetryRequest, str string) error {
	strList := strings.Split(str, " ")
	fmt.Print(len(strList))
	if len(strList) != 18 {
		return &StringSplitError{
			StatusCode: -1,
			Err:        errors.New("not enough string"),
		}
	}
	i.TakimNumarasi, _ = strconv.Atoi(strList[0])
	i.IHAEnlem, _ = strconv.ParseFloat(strList[1], 64)
	i.IHABoylam, _ = strconv.ParseFloat(strList[2], 64)
	i.IHAIrtifa, _ = strconv.ParseFloat(strList[3], 64)
	i.IHADikilme, _ = strconv.Atoi(strList[4])
	i.IHAYatis, _ = strconv.Atoi(strList[5])
	i.IHAHiz, _ = strconv.Atoi(strList[6])
	i.IHABatarya, _ = strconv.Atoi(strList[7])
	i.IHAOtonom, _ = strconv.Atoi(strList[8])
	i.IHAKilitlenme, _ = strconv.Atoi(strList[9])
	i.HedefMerkezX, _ = strconv.Atoi(strList[10])
	i.HedefMerkezY, _ = strconv.Atoi(strList[11])
	i.HedefGenislik, _ = strconv.Atoi(strList[12])
	i.HedefYukseklik, _ = strconv.Atoi(strList[13])
	i.GPSSaati.Saat, _ = strconv.Atoi(strList[14])
	i.GPSSaati.Dakika, _ = strconv.Atoi(strList[15])
	i.GPSSaati.Saniye, _ = strconv.Atoi(strList[16])
	i.GPSSaati.Milisaniye, _ = strconv.Atoi(strList[17])
	return nil
}

func buildLockInfo(i *common.LockInformation, str string) error {
	strList := strings.Split(str, " ")
	if len(strList) != 9 {
		return &StringSplitError{
			StatusCode: -1,
			Err:        errors.New("not enough string"),
		}
	}
	i.LockInit.Hour, _ = strconv.Atoi(strList[0])
	i.LockInit.Minute, _ = strconv.Atoi(strList[1])
	i.LockInit.Second, _ = strconv.Atoi(strList[2])
	i.LockInit.Millisecond, _ = strconv.Atoi(strList[3])
	i.LockEnd.Hour, _ = strconv.Atoi(strList[4])
	i.LockEnd.Minute, _ = strconv.Atoi(strList[5])
	i.LockEnd.Second, _ = strconv.Atoi(strList[6])
	i.LockEnd.Millisecond, _ = strconv.Atoi(strList[7])
	i.IsLockAutonomous, _ = strconv.Atoi(strList[8])
	return nil
}

func TestBuildTelemetryRequest() {
	var tStr string = "0 1.2 2.3 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17"
	fmt.Println(buildTelemetryRequest(&common.TelemReq, tStr))
	fmt.Println(common.TelemReq)
}

func TestBuildLockInfo() {
	var tStr string = "0 1 2 3 4 5 6 7 8"
	fmt.Println(buildLockInfo(&common.LockInfo, tStr))
	fmt.Println(common.LockInfo)
}
