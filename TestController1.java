package com.shareDonation.controller;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

@RestController
@RequestMapping("/data")
public class TestController1 {

    @PostMapping("/data")
    public ResponseEntity<String> receiveData(
            @RequestParam("temperature") String temperature,
            @RequestParam("humidity") String humidity,
            @RequestParam("avgTemperature") String avgTemperature,
            @RequestParam("avgHumidity") String avgHumidity,
            @RequestParam("soundLevel") String soundLevel,
            @RequestParam("obstacleDetection") String obstacleDetection) {

        // 获取当前时间并格式化
        LocalDateTime now = LocalDateTime.now();
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        String formattedNow = now.format(formatter);

        // 打印接收到的数据在同一行
        System.out.println(formattedNow + " | Temperature: " + temperature +
                " | Humidity: " + humidity +
                " | Average Temperature: " + avgTemperature +
                " | Average Humidity: " + avgHumidity +
                " | Sound Level: " + soundLevel +
                " | Obstacle Detection: " + obstacleDetection
        );

        // 处理数据，例如存储到数据库或进一步分析

        // 返回响应
        return ResponseEntity.ok("Data sent successfully");
    }
}
