
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>

#include <HTTP/httplib.h>
#include "HTTP/host_server.hpp"

#include "main.hpp"

#include "JSON/json.hpp"

#include "logging.hpp"
#include "ModConfig.hpp"

#include "CustomTypes/ChatHandler.hpp"
#include "ChatBuilder.hpp"

//std::string GlobalConfigPath() {
//    static std::string path = Configuration::getConfigFilePath(modInfo);
//    return path;
//}

namespace WebServer
{
    bool wsrunning = false;

    std::string loadHtmlFile(const std::string &filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return "<html><body><h1>404 Not Found</h1></body></html>";
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

    const char *html = R"(
            <!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link rel="icon" href="data:image/jpeg;base64,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">
    <title>U-ChatUI</title>
    <style>
      /* Global Reset */
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

/* Body and Background */
html,
body {
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #1d1f27 0%, #14141f 100%);
  font-family: Arial, sans-serif;
  overflow: hidden;
}

.container {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 1000px;
  width:  550px;
  z-index: 2;
}

.form-box {
  background-color: rgb(79, 79, 79);
  padding: 20px;
  border-radius: 50px;
  box-shadow: 0px 0px 15px rgba(255, 255, 255, 0.1);
  color: white;
  max-width: 1000px;
  width: 100%;
  text-align: center;
}

/* Form Elements */
h2 {
  margin-bottom: 50px;
  color: white;
  font-size: 75px;
}

/* Form Elements */
h3 {
  margin-bottom: 30px;
  color: white;
  font-size: 50px;
}

h5 {
  margin: auto;
  color: white;
  font-size: 24px;
}

.token-container {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  margin-top: 10px;
  width: 1000px;
}

.channel {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 10px;
  margin-bottom: -10px;
}

input[type="password"],
input[type="text"] {
  width: calc(100% - 80px);
  padding: 10px;
  border-radius: 5px;
  border: 1px solid #ccc;
  outline: none;
  font-size: 18px;
}

button {
  background:linear-gradient(45deg, rgb(0, 110, 255), violet );
  color: white;
  text-decoration: none;
  border: 1px solid #ccc;
  border-radius: 10px;
  height: 42px;
  width: 100%;
  font-size: large;
}



.save {
  margin-bottom: 10px;
}

a:hover,
button:hover {
  background-color: #005bb5;
}

#token {
  width: 378px;
}

button#toggleToken {
  padding: 10px;
  background-color: #ff0040;
  color: white;
  border: none;
  border-radius: 10px;
  cursor: pointer;
  height: 41px;
  width: 100px;
  margin: 0px;
  font-size: large;
  transform: translateX(-500px);
}

button#submit {
  width: 100%; /* Set to desired width */
  margin-top: 20px;
  background-color: #28a745;
}

button#submit:hover {
  background-color: #218838;
}

/* Background with Grid */
.background {
  position: absolute;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  background: radial-gradient(
      circle at center,
      rgba(0, 0, 0, 0.1),
      rgba(0, 0, 0, 0.7)
    ),
    radial-gradient(circle at center, #3f0099, #002144 90%);
  z-index: 0;
}

.line-bottom {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  width: 100%; /* Width of the line */
  height: 5px; /* Thickness of the line */
  background-color: #ff0000; /* Base color of the line */
  border-radius: 5px; /* Rounded edges */
  box-shadow: 0 0 10px #ff0000, /* Glowing effect */ 0 0 20px #ff0000,
    0 0 30px #ff0000, 0 0 40px #ff0000, 0 0 50px #ff0000, 0 0 60px #ff0000,
    0 0 70px #ff0000;
  animation: flicker-red 2s infinite alternate;
}

.line-top {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  width: 100%; /* Width of the line */
  height: 5px; /* Thickness of the line */
  background-color: #00ffff; /* Base color of the line */
  border-radius: 5px; /* Rounded edges */
  box-shadow: 0 0 10px #00ffff, /* Glowing effect */ 0 0 20px #00ffff,
    0 0 30px #00ffff, 0 0 40px #00ffff, 0 0 50px #00ffff, 0 0 60px #00ffff,
    0 0 70px #00ffff;
  animation: flicker-blue 2s infinite alternate;
}

@keyframes flicker-blue {
  0% {
    opacity: 1;
    box-shadow: 0 0 10px #00ffff, 0 0 20px #00ffff, 0 0 30px #00ffff,
      0 0 40px #00ffff, 0 0 50px #00ffff, 0 0 60px #00ffff, 0 0 70px #00ffff;
  }
  50% {
    opacity: 0.8;
    box-shadow: 0 0 5px #00ffff, 0 0 15px #00ffff, 0 0 20px #00ffff,
      0 0 30px #00ffff, 0 0 40px #00ffff;
  }
  100% {
    opacity: 1;
    box-shadow: 0 0 10px #00ffff, 0 0 20px #00ffff, 0 0 30px #00ffff,
      0 0 40px #00ffff, 0 0 50px #00ffff, 0 0 60px #00ffff, 0 0 70px #00ffff;
  }
}

@keyframes flicker-red {
  0% {
    opacity: 1;
    box-shadow: 0 0 10px #ff0000, 0 0 20px #ff0000, 0 0 30px #ff0000,
      0 0 40px #ff0000, 0 0 50px #ff0000, 0 0 60px #ff0000, 0 0 70px #ff0000;
  }
  50% {
    opacity: 0.8;
    box-shadow: 0 0 5px #ff0000, 0 0 15px #ff0000, 0 0 20px #ff0000,
      0 0 30px #ff0000, 0 0 40px #ff0000;
  }
  100% {
    opacity: 1;
    box-shadow: 0 0 10px #ff0000, 0 0 20px #ff0000, 0 0 30px #ff0000,
      0 0 40px #ff0000, 0 0 50px #ff0000, 0 0 60px #ff0000, 0 0 70px #ff0000;
  }
}


    </style>
  </head>
  <body>
    <div class="container">
      <div class="form-box">
        <h2>Settings</h2>
        <form>
          <h3>Twitch Token</h3>
          <h5>Add twitch token to enable twitch functionality</h5>
          <div class="token-container">
            <input
              type="password"
              id="token"
              placeholder="Enter your token"
              required
              data-hidden="true"
            />
            <button type="button" id="toggleToken", class="toggleToken">Show</button>
          </div>
          <button
            type="button"
            href="https://id.twitch.tv/oauth2/authorize?client_id=23vjr9ec2cwoddv2fc3xfbx9nxv8vi&redirect_uri=http%3A%2F%2Flocalhost%3A8339%2F%3Ftype%3Dchat&response_type=token&scope=bits:read+channel:manage:broadcast+channel:manage:polls+channel:manage:moderators+channel:manage:predictions+channel:manage:redemptions+channel:moderate+channel:read:redemptions+channel:read:hype_train+channel:read:predictions+channel:read:polls+channel:read:subscriptions+chat:edit+chat:read+clips:edit+moderator:manage:banned_users+moderator:read:followers+whispers:edit+whispers:read"
            id="newToken"
            class="newToken"
            >Get new token!</button
          >
          <input
            type="text"
            id="channel"
            class="channel"
            placeholder="Enter your channel"
            required
          />
        </form>
        <button type="button" id="submit", class="save">Save</button>
      </div>
    </div>
    <div class="background">
      <div class="line-top"></div>
      <div class="line-bottom"></div>
    </div>
    <div class="con">
      <div class="box"></div>
      <div class="box"></div>
      <div class="box"></div>
      <div class="box"></div>
      <div class="box"></div>
      <div class="box"></div>
    </div>
  </div>

    <script>
      // Toggle the visibility of the token
const toggleToken = document.getElementById("toggleToken");
const tokenInput = document.getElementById("token");

toggleToken.addEventListener("click", function () {
  // Check if the token input is currently hidden (using a text mask)
  const isHidden = tokenInput.getAttribute("data-hidden") === "true";

  // Toggle the mask on the input
  if (isHidden) {
    tokenInput.setAttribute("type", "text"); // Show the token
    tokenInput.setAttribute("data-hidden", "false");
    this.textContent = "Hide";
  } else {
    tokenInput.setAttribute("type", "password"); // Hide the token by changing input type
    tokenInput.setAttribute("data-hidden", "true");
    this.textContent = "Show";
  }
});

// Function to generate a random token
function generateToken(length = 32) {
  const charset =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  let token = "";
  for (let i = 0; i < length; i++) {
    const randomIndex = Math.floor(Math.random() * charset.length);
    token += charset[randomIndex];
  }
  return token;
}

// Add event listener to the button
document.getElementById("submit").addEventListener("click", function () {
  try {
    const tokenInput = document.getElementById("token");
    const channel = document.getElementById("channel");

    const data = {
      channelName: channel.value,
      token: tokenInput.value,
    };

    // Call the server to send the messages
    fetch("http://localhost:3000/get-data", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    }).catch(console.error);
    console.log("send");
  } catch (e) {
    console.error(e);
  }
});


const con = document.querySelector("con");

function qubes() {
  for (let i = 0; i < 20; i++) {
    const box = document.createElement("div");
    box.classList.add('box');
    box.style.width = `${Math.random() * 100 + 20}px`;
    box.style.height = `${Math.random() * 100 + 20}px`;
    box.style.left = `${Math.random() * 100}vw`;
    box.style.top = `${Math.random() * 100}vh`;
    box.style.animationDuration = `${Math.random() * 10 + 5}s`;
    box.style.animationDelay = `${Math.random() * 5}s`;

    container.appendChild(box);
  }
}

createFloatingBoxes();
    </script>
    <!-- Link to JavaScript file -->
  </body>
</html>

            )";

    void start()
    {
        if (wsrunning)
            return;
        wsrunning = true;

        new std::thread([]()
                        {
            httplib::Server server;

            
            server.Get("/", [](const httplib::Request & /*req*/, httplib::Response &res) {
    res.set_content(html, "text/html");
  });


            server.Get("/hello", [](const httplib::Request & /*req*/, httplib::Response &res) {
    res.set_content("hello!", "text/plain");
  });


            server.Post("/recieve_logindetails", [](const httplib::Request & req, httplib::Response &res) {
    try {
        INFO("ChatUI - Recieved login details.. parsing JSON");
        auto jsonData = nlohmann::json::parse(req.body);
        std::string twitchtoken = jsonData["Token"];
        std::string twitchusername = jsonData["Username"];
        std::ofstream TwitchToken(GlobalConfigPath());
        std::ofstream TwitchUsername(GlobalConfigPath());
        TwitchToken << twitchtoken;
        TwitchToken.close();
        TwitchUsername << twitchusername;
        TwitchUsername.close();
        res.set_content("100", "text/plain");
        getModConfig().Channel.SetValue(twitchusername);


        // Code 100 means read success
        // If it's completely invalid, it won't be accepted.

    } catch (const std::exception& e) {
        INFO("ChatUI - Bad Request has been made for recieve_logindetails");
        res.status = 400;  // Bad request
        res.set_content("Invalid JSON (recieve_logindetails): " + std::string(e.what()), "text/plain");
    }
  });


            // Start the server on port 4444
            if (!server.listen("localhost", 4444)) {
                //
        } });
    }
}