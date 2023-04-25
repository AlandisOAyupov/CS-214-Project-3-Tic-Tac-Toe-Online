Alandis Ayupov - aoa101
Veer Advaney - vsa26

Implementation Choice - Concurrent Games with Interruption

Test Plan/Testing Considerations

Test Method - All tests were done manually via the client. Examples of inputs are listed below.

1. Program connects multiple clients to a server. (Done)
2. PLAY command works. Along with DRAW and RSGN (Done)
Example:
PLAY|4|Joe| works. 
RSGN|0| works.
DRAW|2|S| works, while DRAW|2|R| and DRAW|2|A| works. Players cannot accept or reject their own DRAW requests.
3. Commands with improper formatting are invalid. (Done)
Examples:
 PLAY|5|Joe| - The field containing the length of remaining bytes is incorrect. (INVL|16|Invalid format|)
 MFTH|4|Joe| - not a recognized command. (INVL|16|Invalid format|)
 PLAY,4|Joe| or PLAY|4,Joe| or PLAY|4|Joe, (etc) - improper field separation. (INVL|16|Invalid format|)
 PLAY|257|(Insert 257 Characters here)| - Command too long. 
4. Sides are chosen randomly. A player can be either an X or an O. (Done)
5. MOVE makes a change to the board. In addition, MOVE errors are detected. (Done)
Examples:
 MOVE|6|X|2,2| - when row 2 col 2 is already occupied.
 MOVE|6|X|2,2| - when the player is on the opposite side.
 MOVE|6|X|4,4| - not valid coordinates.
 MOVE|6|X|3l3| or MOVE|6|Xl3,3| - improper formating.
6. A turn-based order is established an followed. Moves are not played in an improper order. Certain commands being played like DRAW|2|S|
do not mess with the already established turns. (Done)
Example:
MOVE|6|O|2,2| Does not work as the very first move on the board, but does as the second move. (If 2,2 is not occupied)
7. Certain commands do not work when played at the wrong time. (Done)
Examples:
 PLAY command when already in a game.
 DRAW|2|R| or DRAW|2|A| when a draw has not been sent, or DRAW|2|S| when the opposing player already proposed a draw.
 As mentioned previously, the MOVE command when the player sent a draw offer but hasn't made a move yet.
8. Server messages are picked up by the client, and work fine. (Done)
Examples:
 Invalid is sent when the command is invalid.
 OVER command is displayed when a game is won/lost.
 MOVD is displayed when a move is made.
 WAIT is displayed after PLAY
 etc...
9. Server can handle multiple clients, and multiple games at once, thus fufilling the "concurrent" requirement. (Done)
10. In addition, when a game is over, or a client disconnects, the game terminates properly. ./ttt in our implementation exits the process.
11. Servers can handle two messages at once or partial messages from the client. (Done)
Examples: 
MOVE
|6|X|2,2| should still work as long as it is Player X's turn and 2,2 is not occupied. 
Same goes for DRAW|2|S|MOVE|6|X|2,2|
12. Player can not take an existing username of a player. In our implementation, once a game ends, the username is free for the taking,
as the game contents are cleared and the struct for games in our implementation allows the space to be reused. 
13. Messages can be sent at any time, thus fulfilling the "interruption" requirement. In our implementation, Any command can be played, 
however if a person plays a MOVE when it is not their turn, they get an INVL message in response. 
