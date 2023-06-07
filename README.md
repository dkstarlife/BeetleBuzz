# BeetleBuzz

BeetleBuzz is a simple chat bot for Twitch.tv, written in C++.

This is a small recreational coding project and as such may have messy code, and it may not get many future updates.
Feel free to use the code for what ever you may feel like.

For specifics on how Twitch's API works see https://dev.twitch.tv/docs/irc/

## Dependencies
[Boost][BoostWebsite] for websocket.

[OpenSSL][OpenSSLWebsite] for SSL encryption.

[BoostWebsite]: https://www.boost.org
[OpenSSLWebsite]: https://www.openssl.org

## Funtionality
Current implemented commands (not case sensitive):
- `!Dice x ...` takes 0 or more numbers and rolls an x-sided die for each one, and sends the rolls back a a response in chat. If no numbers are given it defaults to a single six-sided die.
- `!MyColor` picks a named color from a list using a hash of the caller's username, and reponds with the color name and hex value.
- `!MyElement` picks an element from a list using a hash of the caller's username, and responds with it. The options are: 
      💨 Air 💨, 🌫 Mist 🌫, 🌪 Dust 🌪,
			⛰ Earth ⛰, ⏳ Sand ⏳, 🔩 Metal 🔩,
			🔥 Fire 🔥, 🌋 Lava 🌋, ⚡ Lightning ⚡,
			🌊 Water 🌊, ☁ Steam ☁, ❄ Ice ❄,
      🌌 Aether 🌌
- `!MyFortune` picks a fortune cookie fortune from a list, using a hash of the day and callers username.
- `PickOne a | b | c ...` takes 2 or more options separated by '|' symbols, and randomly picks one and responds with it. 
- `!MyAnimal` picks a random animal emoji based on the caller's name, and responds with it.
