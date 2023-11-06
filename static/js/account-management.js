var selectedAccountId = null; // 전역 변수로 선택된 계좌 ID를 저장합니다.

document.addEventListener('DOMContentLoaded', function() {
    var accountsListEl = document.getElementById('accounts-list');
    var balanceEl = document.getElementById('balance-amount');

    function updateFinancialData() {
        fetch('/getUserFinancialData') // 변경된 엔드포인트
            .then(function(response) {
                return response.json();
            })
            .then(function(data) {
                // 계좌 목록 업데이트
                accountsListEl.innerHTML = '';
                data.bankAccounts.forEach(function(account) {
                    var accountEl = document.createElement('div');
                    accountEl.className = 'account-item';
                    accountEl.dataset.id = account.id;
                    accountEl.innerHTML = `<strong>${account.accountName}</strong> (Balance: $${account.balance.toFixed(2)})`;
                    accountEl.onclick = function() {
                        selectAccount(account.id);
                    };
                    accountsListEl.appendChild(accountEl);
                });

                // 총 잔액 업데이트
                balanceEl.textContent = `$${data.totalBalance.toFixed(2)}`; // 잔액 표시
            })
            .catch(function(error) {
                console.error('Error fetching financial data:', error);
            });
    }

    function selectAccount(accountId) {
        selectedAccountId = accountId;
        var accountItems = document.querySelectorAll('.account-item');
        accountItems.forEach(function(item) {
            if (item.dataset.id == selectedAccountId) {
                item.classList.add('selected-account');
            } else {
                item.classList.remove('selected-account');
            }
        });
    }

    updateFinancialData();

    function performTrade(action) {
        if (selectedAccountId == null) {
            alert('Please select an account first.');
            return;
        }

        var amount = document.getElementById('amount').value;
        if(amount <= 0) {
            alert('Please enter a valid amount.');
            return;
        }

        var params = new URLSearchParams();
        params.append('action', action);
        params.append('amount', amount);
        params.append('accountId', selectedAccountId);

        fetch('/profile_action', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: params
        }).then(function(response) {
            return response.text();
        }).then(function(text) {
            console.log(text);
            updateFinancialData();
            selectedAccountId = null;
            alert(text); // Display the result from the server
        }).catch(function(error) {
            console.error('Error:', error);
        });
    }

    // Event listeners for deposit and withdrawal buttons
    document.getElementById('deposit').addEventListener('click', function() {
        performTrade('deposit');
    });

    document.getElementById('withdrawal').addEventListener('click', function() {
        performTrade('withdraw');
    });

});
