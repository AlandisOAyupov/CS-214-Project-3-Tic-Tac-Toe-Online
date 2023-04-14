Alandis Ayupov - aoa101
Veer Advaney - vsa26

Test Plan/Testing Considerations
1. Program connects two clients to a server. (Done)
2. PLAY command works. (Done)
3. Commands with improper formating are invalid. (Done)
Examples:
 PLAY|5|Joe| - Field containing length of remaining bytes is incorrect. (INVL|16|Invalid format|)
 MFTH|4|Joe| - not a recognized command. (INVL|16|Invalid format|)
 PLAY,4|Joe| or PLAY|4,Joe| or PLAY|4|Joe, (etc) - improper field seperation. (INVL|16|Invalid format|)
 PLAY|257|(Insert 257 Characters here)| - Command too long. 
4. Sides are chosen randomly. A player can both be either an X or an O. (Done)
5. MOVE makes a change to the board. In addition, MOVE errors are detected.
Examples:
 MOVE|6|X|2,2| - when row 2 col 2 is already occupied.
 MOVE|6|X|2,2| - when player is on the opposite side.
 MOVE|6|X|4,4| - not valid coordinates.
 MOVE|6|X|3l3| or MOVE|6|Xl3,3| - improper formating.
6. A turn-based order is established an followed. Moves are not played in improper order. Certain commands being played like DRAW|2|S|
do not mess with the already estblished turnes. (Done)
7. In our implementation, DRAW|2|S| ends a turn, however the opposing player must reject the draw, accept the draw, or resign. This makes
it so that moves are still played in order.
8. Certain commands do not work when played at the wrong time.
Examples:
 PLAY command when already in a game.
 DRAW|2|R| or DRAW|2|A| when a draw has not been sent, or DRAW|2|S| when the opposing player already proposed a draw. 
 As mentioned previously, MOVE command when the player sent a draw offer but hasn't made a move yet.
9. Server messages are picked up by the client, and work fine.
Examples:
 Invalid is sent when command is invalid.
 OVER command is displayed when a game is won/lost.
 MOVD is displayed when a move is made.
 WAIT is displayed after PLAY
 etc...
