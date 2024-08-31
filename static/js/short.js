function updateLinks() {
    const links = document.querySelector("#links");

    fetch("/api/short")
        .then((data) => data.json())
        .then((data) => {
            if ("error" in data) {
                const text = document.createElement("span");
                text.textContent = data.error;
                text.style.color = "red";
                links.replaceChildren(text);
            } else {
                let elements = []

                for (const entry of data.links) {
                    const mnemonic = document.createElement("span")
                    mnemonic.className = "linkelem";
                    mnemonic.textContent = entry.mnemonic;
                    elements.push(mnemonic);

                    const link_div = document.createElement("div");
                    link_div.className = "linkelem";
                    const link = document.createElement("a");
                    link.className = "link";
                    link.textContent = entry.link;
                    link.href = entry.link;
                    link_div.appendChild(link);
                    elements.push(link_div);

                    const button_div = document.createElement("div");
                    button_div.className = "linkelem";
                    const button = document.createElement("button");
                    button.className = "linkbutton";
                    button.dataset.mnemonic = entry.mnemonic;
                    button.textContent = "Delete";
                    button_div.appendChild(button);
                    elements.push(button_div);
                }

                links.replaceChildren(...elements);
            }
        })
        .catch((error) => {
            const text = document.createElement("span");
            text.textContent = "An error has occurred: " + error.toString();
            text.style.color = "red";
            response.replaceChildren(text);
        });
}

function submitLink() {
    const url = document.querySelector("#url");
    const response = document.querySelector("#response");

    console.log(url.value);
    if (url.value.length < 1 || url.value.length > 1500) {
        return;
    }

    fetch("/api/short", {
        "method": "POST",
        "body": JSON.stringify({ "link": url.value })
    })
        .then((data) => data.json())
        .then((data) => {
            const text = document.createElement("span");
            if ("error" in data) {
                text.textContent = data.error;
                text.style.color = "red";
            } else {
                const href = document.createElement("a");
                href.href = `https://boolco.dev/s/${data.mnemonic}`;
                href.textContent = href.href;
                text.append("Your link has been created! Visit ", href, " to access it.");
                text.style.color = "green";
            }
            response.replaceChildren(text);
            updateLinks();
        })
        .catch((error) => {
            const text = document.createElement("span");
            text.textContent = "An error has occurred: " + error.toString();
            text.style.color = "red";
            response.replaceChildren(text);
        });
}

window.onload = function () {
    const button = document.querySelector("#button");
    button.addEventListener("click", submitLink);
    updateLinks();
}