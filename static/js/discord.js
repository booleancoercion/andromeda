function updateNames(list = null) {
    const names = document.querySelector("#names");
    if (list === null) {
        names.replaceChildren();
    } else {
        let elems = [];
        for (const name of list) {
            const word = document.createElement("span");
            word.className = "word";
            word.textContent = name;
            elems.push(word);
        }
        names.replaceChildren(...elems);
    }
}

function submitName() {
    const name = document.querySelector("#name");
    const response = document.querySelector("#response");

    if (name.value.length < 3 || name.value.length > 50) {
        return;
    }

    fetch("/api/discord/" + encodeURIComponent(name.value))
        .then((data) => data.json())
        .then((data) => {
            const text = document.createElement("span");
            if ("error" in data) {
                text.textContent = data.error;
                text.style.color = "red";
                response.replaceChildren(text);
                updateNames();
            } else if ("names" in data) {
                response.replaceChildren();
                updateNames(data.names);
            }
        })
        .catch((error) => {
            const text = document.createElement("span");
            text.textContent = "An error has occurred: " + error.toString();
            text.style.color = "red";
            response.replaceChildren(text);
            updateNames();
        });
}

window.onload = function () {
    const button = document.querySelector("#button");
    button.addEventListener("click", submitName);
}