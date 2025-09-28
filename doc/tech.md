## Tech Details

### Component Relations

```
1. Relay:
	exe -> core -> netmill
		<-      <-

2. Client:
	exe/jni -> core -> phiola -> modules
		    <-      <-
```


### Caller-Relay-Callee Diagram

```
CALLER                    RELAY                       CALLEE
============================================================
                            . <-( /login?name=... )-- *       [1.1] Callee registers on Relay server
                            . --( 200 )->             .

                            . <-( /ready )--          *       [1.2] Callee indicates it's ready to accept calls

* --( /login?name=... )-> .                                   [2.1] Caller registers on Relay server
. <-( 200 )--             .

* --( /call?name=... )->  .                                   [2.2] Caller initiates the call
                            . --( 200 )->             .       [1.3] Server notifies the callee 
. <-( 200 )--             .                                   [2.3] Server notifies the caller

* <-( audio )->           <---->        <-( audio )-> *       [3] Both peers transfer audio data
```


### phiola track chain during the call

```
adev.rec -> af.noise-gate -> af.gain -> af.aconv -> af.soxr -> ac-opus.enc -> fmt.ogg.w -> zvon.send  [Peer#1 Recording Track]
->(network)->
zvon.recv -> ogg.r -> ac-opus.dec -> af.soxr -> af.aconv -> adev.play                                 [Peer#2 Playback Track]
```
