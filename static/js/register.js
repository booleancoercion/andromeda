function generateToken() {
    const generated_token = document.querySelector("#generated_token");

    fetch("/api/generate_registration_token", {
        method: "POST"
    })
        .then((data) => data.json())
        .then((data) => {
            if ("error" in data) {
                generated_token.textContent = "An error has occurred: " + data.error;
            } else if ("token" in data) {
                generated_token.textContent = data.token;
            }
        })
        .catch((error) => {
            generated_token.textContent = "An error has occurred: " + error.toString();
        });
}

window.onload = function () {
    const button = document.querySelector("#generate_token");
    button.addEventListener("click", generateToken);
}