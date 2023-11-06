function fetchStockData() {
    fetch('/api/stocks')
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok: ' + response.statusText);
            }
            return response.json();
        })
        .then(data => {
            const tableBody = document.getElementById('stocksTable').querySelector('tbody');
            const labels = [];
            const chartData = [];
            const backgroundColors = [
                'rgb(255, 99, 132)', 'rgb(255, 159, 64)', 'rgb(255, 205, 86)',
                'rgb(75, 192, 192)', 'rgb(54, 162, 235)', 'rgb(153, 102, 255)'
            ];

            tableBody.innerHTML = ''; // Clear the table body to refresh the data

            data.forEach((stock, index) => {
                let row = tableBody.insertRow();
                row.insertCell(0).textContent = stock.companyName;
                row.insertCell(1).textContent = stock.sharesOwned;
                row.insertCell(2).textContent = `$${stock.totalValue.toFixed(2)}`;

                let returnDollarCell = row.insertCell(3);
                let returnPercentCell = row.insertCell(4);

                returnDollarCell.textContent = `$${stock.returnDollars.toFixed(2)}`;
                returnPercentCell.textContent = `${stock.returnPercent.toFixed(2)}%`;

                // Color the return values based on positive or negative
                if (stock.returnDollars >= 0) {
                    returnDollarCell.classList.add('positive');
                } else {
                    returnDollarCell.classList.add('negative');
                }

                if (stock.returnPercent >= 0) {
                    returnPercentCell.classList.add('positive');
                } else {
                    returnPercentCell.classList.add('negative');
                }

                // Prepare the chart data
                labels.push(stock.companyName);
                chartData.push(stock.totalValue);
            });

            // Update the chart
            updateChartData(labels, chartData, backgroundColors);
        })
        .catch(error => console.error('Error fetching stock data:', error));
}


function updateChartData(labels, data, backgroundColors) {
    let ctx = document.getElementById('pieChartCanvas1').getContext('2d');

    // If the chart already exists, destroy it so we can create a new one
    if (window.pieChart) window.pieChart.destroy();

    window.pieChart = new Chart(ctx, {
        type: 'pie',
        data: {
            labels: labels,
            datasets: [{
                data: data,
                backgroundColor: backgroundColors.slice(0, data.length) // Match the number of colors to the data length
            }]
        },
        options: {
            responsive: false,
            maintainAspectRatio: true
        }
    });
}

document.addEventListener('DOMContentLoaded', function () {
    refreshData(); // Call this function to refresh data when the DOM is fully loaded
});

function refreshData() {
    fetchStockData();
    // Refresh the data every 60 seconds (60000 milliseconds)
    setTimeout(refreshData, 60000);
}
