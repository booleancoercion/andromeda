function timeSince(date) {
    if (typeof date !== 'object') {
        date = new Date(date);
    }

    const seconds = Math.floor((new Date() - date) / 1000);
    let intervalType;

    let interval = Math.floor(seconds / 31536000);
    if (interval >= 1) {
        intervalType = 'year';
    } else {
        interval = Math.floor(seconds / 2592000);
        if (interval >= 1) {
            intervalType = 'month';
        } else {
            interval = Math.floor(seconds / 86400);
            if (interval >= 1) {
                intervalType = 'day';
            } else {
                interval = Math.floor(seconds / 3600);
                if (interval >= 1) {
                    intervalType = "hour";
                } else {
                    interval = Math.floor(seconds / 60);
                    if (interval >= 1) {
                        intervalType = "minute";
                    } else {
                        interval = seconds;
                        intervalType = "second";
                    }
                }
            }
        }
    }

    if (interval > 1 || interval === 0) {
        intervalType += 's';
    }

    return interval + ' ' + intervalType;
};

function showMessages() {
    const messages = document.querySelector("#messages");

    fetch("/api/game")
        .then((data) => data.json())
        .then((data) => {
            if ("error" in data) {
                const text = document.createElement("span");
                text.textContent = data.error;
                text.style.color = "red";
                messages.replaceChildren(text);
            } else {
                const dl = document.createElement("dl");
                for (const [index, message] of data.messages.entries()) {
                    const dt = document.createElement("dt");
                    const code = document.createElement("code");
                    code.textContent = message.name;
                    dt.appendChild(code);
                    dt.appendChild(document.createTextNode(` says: (${timeSince(message.timestamp)} ago)`));
                    dt.style.opacity = 1 - index / 10;

                    const dd = document.createElement("dd");
                    dd.textContent = message.content;
                    dd.style.opacity = 1 - index / 10;

                    dl.appendChild(dt);
                    dl.appendChild(dd);
                }

                messages.replaceChildren(dl);
            }
            messages.style.height = messages.scrollHeight + "px";

        })
        .catch((error) => {
            const text = document.createElement("span");
            text.textContent = "An error has occurred: " + error.toString();
            text.style.color = "red";
            messages.replaceChildren(text);
            messages.style.height = messages.scrollHeight + "px";
        });
}

function submitMessage() {
    const name = document.querySelector("#name");
    const content = document.querySelector("#content");
    const response = document.querySelector("#response");

    if (name.value.length < 1 || name.value.length > 40 ||
        content.value.length < 1 || content.value.length > 1000) {
        return;
    }

    fetch("/api/game", {
        "method": "POST",
        "body": JSON.stringify({
            "name": name.value,
            "content": content.value
        })
    })
        .then((data) => data.json())
        .then((data) => {
            const text = document.createElement("span");
            if ("success" in data) {
                text.textContent = data.success;
                text.style.color = "green";
            } else if ("error" in data) {
                text.textContent = data.error;
                text.style.color = "red";
            }
            response.replaceChildren(text);
            if ("success" in data) {
                showMessages();
            }
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
    button.addEventListener("click", submitMessage);

    showMessages();
}