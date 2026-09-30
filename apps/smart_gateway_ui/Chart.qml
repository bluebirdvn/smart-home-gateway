
import QtQuick
import QtCharts

Item {
    id: chartRoot

    property real currentTemp: 0.0
    property real currentHumi: 0.0
    property real currentLux: 0.0
    property int tick: 0

    ChartView {
        id: chartView
        anchors.fill: parent
        antialiasing: true
        theme: ChartView.ChartThemeLight
        backgroundColor: "#F5F7F8"
        backgroundRoundness: 0
        legend.alignment: Qt.AlignTop
        legend.labelColor: "#263238"
        legend.font: Qt.font({ pixelSize: 8 })
        margins.top: 2
        margins.bottom: 2
        margins.left: 2
        margins.right: 2

        ValueAxis {
            id: axisX
            min: 0
            max: 30
            titleText: "Time(s)"
            labelsColor: "#37474F"
            gridLineColor: "#D0D5DA"
            labelsFont: Qt.font({ pixelSize: 8 })
            titleFont: Qt.font({ pixelSize: 8 })
        }
        ValueAxis {
            id: axisY
            min: 0
            max: 100
            titleText: "Value"
            labelsColor: "#37474F"
            gridLineColor: "#D0D5DA"
            labelsFont: Qt.font({ pixelSize: 8 })
            titleFont: Qt.font({ pixelSize: 8 })
        }

        LineSeries {
            id: tempSeries
            name: "Temp (°C)"
            axisX: axisX
            axisY: axisY
            color: "#D84315"
            width: 2
        }
        LineSeries {
            id: humiSeries
            name: "Humi (%)"
            axisX: axisX
            axisY: axisY
            color: "#0277BD"
            width: 2
        }
        LineSeries {
            id: luxSeries
            name: "Lux"
            axisX: axisX
            axisY: axisY
            color: "#F9A825"
            width: 2
        }
    }

    Timer {
        id: chartTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            chartRoot.tick++
            tempSeries.append(chartRoot.tick, chartRoot.currentTemp)
            humiSeries.append(chartRoot.tick, chartRoot.currentHumi)
            luxSeries.append(chartRoot.tick, chartRoot.currentLux)
            if (chartRoot.tick > axisX.max) {
                axisX.min++
                axisX.max++
            }
        }
    }

    function clearChart() {
        tempSeries.clear()
        humiSeries.clear()
        luxSeries.clear()
        chartRoot.tick = 0
        axisX.min = 0
        axisX.max = 30
    }
}
