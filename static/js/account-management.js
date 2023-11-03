document.addEventListener('DOMContentLoaded', function() {
    var bankAccounts = [
        { id: 1, accountName: 'Savings Account', balance: 1200.00 },
        { id: 2, accountName: 'Checking Account', balance: 150.50 }
    ];

    // Variable to keep track of the selected account
    var selectedAccountId = null;

    var accountsListEl = document.getElementById('accounts-list');

    function updateAccountsList() {
        accountsListEl.innerHTML = '';
        bankAccounts.forEach(function(account) {
            var accountEl = document.createElement('div');
            accountEl.className = 'account-item';
            accountEl.dataset.id = account.id;
            accountEl.innerHTML = `<strong>${account.accountName}</strong> (Balance: $${account.balance.toFixed(2)})`;
            accountEl.onclick = function() {
                selectAccount(account.id);
            };
            accountsListEl.appendChild(accountEl);
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

    updateAccountsList();

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
            alert(text); // Display the result from the server
        }).catch(function(error) {
            console.error('Error:', error);
        });
    }

    document.getElementById('deposit').addEventListener('click', function() {
        performTrade('deposit');
    });

    document.getElementById('withdrawal').addEventListener('click', function() {
        performTrade('withdraw');
    });

});
