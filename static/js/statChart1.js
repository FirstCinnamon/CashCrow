// 통계 현황 페이지 파이 차트 1

let pieChartData1 = {
    labels: ['foo', 'bar', 'baz', 'fie', 'foe', 'fee'], 
    datasets: [{ 
        data: [95, 12, 13, 7, 13, 10], // 파이 차트에 들어갈 데이터 형태(list)
        backgroundColor: ['rgb(255, 99, 132)', 'rgb(255, 159, 64)', 'rgb(255, 205, 86)', 'rgb(75, 192, 192)', 'rgb(54, 162, 235)', 'rgb(153, 102, 255)'] // 데이터 배경 색
    }] 
};

let pieChartDraw1 = function () {
    let ctx = document.getElementById('pieChartCanvas1').getContext('2d');
    
    window.pieChart = new Chart(ctx, {
        type: 'pie',
        data: pieChartData1,
        options: {
            responsive: false,
            maintainAspectRatio: true // true일시 캔버스 크기에 따라 리사이징됨
        }
    });
};
