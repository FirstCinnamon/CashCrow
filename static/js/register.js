function encodeCredentials() {
    const username = document.getElementById("username").value;
    const password = document.getElementById("password").value;
    const credentials = `${username}:${password}`;
    const base64Credentials = btoa(credentials);
    alert(`${base64Credentials}`)
    return `Basic ${base64Credentials}`;
}