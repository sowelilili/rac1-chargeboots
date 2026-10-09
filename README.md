# Charge Boots, strafing and RC2 movement for Ratchet & Clank HD (NPEA00385), as a RaCMAN mod

RC2's Charge Boots and strafing, ported from [Joe-GH-18/RAC-1-Mods](https://github.com/Joe-GH-18/RAC-1-Mods)
and rewritten in C, so that RaCMAN can write them into the running game instead of patching the EBOOT. Also
RC2's walking (speed, turning, quick turn, stopping) and RC2's camera speed and follow. Behaviour around the charge follows RC2 with the
Black Label movement re-patch (isaka & robo).

## What it does

**The Charge Boots are the Grindboots' item.** No speedrun category uses the Grindboots, so the Charge Boots take their place.
- **Getting them:** buy them on Blarg from the Scrawny Scientist for 1,000 bolts (2,000 in the unmodded game), which is the only place the game gives that item.
- **In the save:** the save keeps them. An existing save that already has the Grindboots has the Charge Boots.
- **Grinding:** owning them still lets Ratchet grind on rails.
- **Names:** the game's texts call them Charge Boots in English, in every language: the purchase prompt and pop-up, the Foot Items name and description in the pause menu, and the "Buy" mission.
- **Still Grindboots:** the help messages about them and about rails (voiced), and their icon and model.
- **Challenge mode:** keeps them, as it keeps the PDA.
- **Without them:** there is no charge. Everything else in the mod still works.

**Charging.** Tap R1 twice and hold. A charge can start from standing, walking, crouching,
skidding (including the skid right after a charge) or on landing.
- **Not out of look mode:** as in RC2, a charge never starts in first-person look mode (L1), whatever is in Ratchet's hand.
  - With a weapon in hand (a glove, a gun, the Devastator, the RYNO, ...), RC1's look mode is a state of its own (0x1E), which the charge used to allow. With L1 held that was an easy buffer for charges in the air.
  - There is no crouching inside look mode either, as in RC2. RC1 crouches while R1 is held, but only as an animation; Ratchet stays in look mode.
  - Letting go of L1 with R1 held still crouches, as in RC1. A charge can't start from that crouch.
- It uses RC2's numbers: 30 units/s for the first second, steering at a tenth, then 11.5.
- It has RC2's animation, sounds and foot trails.
- **Camera:** it snaps behind him and follows closely, as RC2's does during a charge.
  - It turns to behind him within a frame or two, eased, at up to 45 degrees a frame.
  - Its look-at point follows on a stiffer spring.
  - It pulls in to a distance of 3.
  - The right stick still turns it.
- It breaks crates in front of Ratchet and bumps him off walls.
- **Height:** as in RC2, he hovers 0.17 above the ground with no gravity. Over a gap the hover sinks towards the ground below at up to 7.5 units/s; if no ground is found, it aims for height 0.17 in the world.
- **Mud:** charging onto mud sinks him at once, ending the charge, as in RC2. The same goes for lava and other deadly layers.
  - RC1's mud only takes Ratchet while he is moving down, and the hover moves his height outside the movement it checks.
  - RC2 waives that check during a charge, and so does this mod.
- **Ice and water:** a charge starts on ice or wading as on any ground, as in RC2. One that ends on ice goes on sliding in RC1's ice state at the charge's speed, as RC2's does in its own.
- **Slopes:** as in RC2, he moves level, and the hover and the ground snap follow the ground.
  - The ground snap uses RC2's spring, which pushes Ratchet out of the ground at up to 5 units/s instead of RC1's 2. This applies everywhere, not only during a charge.
  - On upslopes steeper than about 26 degrees he sinks part way in until he bumps off, as in RC2.
- **Body lean:** he leans into the steering through RC2's springs on his body joints, which stay as the charge left them until the walk or a jump sets its own.
- **Timing:** as in RC2, the charge starts behind the frame's own movement, so he first moves at charge speed on the next frame.

**Ending a charge.** A charge ends into a skid that keeps Ratchet's speed when you:
- let go of R1 after the first second;
- step onto a Magneboots surface;
- use a weapon.

**Weapons during a charge.**
- **Ending it:** using a weapon ends the charge on the next frame. RC2 does the same, because its weapons fire after its charge transitions have run.
- **Gloves** (Bomb, Mine, Decoy, Glove of Doom, Drones) are thrown on the move, whatever the stick does.
- **Devastator:** it always counts as using a weapon during a charge. Normally RC1 only plays its animation when the stick is below 0.7.
- **Guns and the RYNO:** guns (Blaster, Pyrocitor, Tesla Claw, Suck Cannon, Morph-O-Ray) and the RYNO fire, and that ends the charge too.
- **PDA:** it opens as usual, and the charge goes on.
- **No effect:** the wrench, Swingshot, Walloper and Hologuise do nothing during a charge, as in RC2.
- **Never fire:** the Visibomb, because it needs Ratchet on the ground. The Taunter never counts as using a weapon.

**After a charge (Black Label rules).**
- **Lock.** For 30 frames the skid blocks jumping, crouching and walking off. Throwing, swinging the wrench, shooting and charging again all work from the first frame. Unpatched RC2 hides every button from Ratchet for those 30 frames; Black Label removes that, and RC1 never had it.
- **Glove into throw.** A glove that ends a charge sends Ratchet straight on into the glove's standing throw, still sliding. As in RC2, that only happens when all of these hold:
  - the glove has been out for more than 22 frames;
  - Circle was pressed in the last 7 frames or is held;
  - there is ammo left.

  Otherwise the glove acts like a weapon fired once: its throw plays out, and he keeps skidding. A single R1 press can then start a new charge, midair too. Two examples:
  - a glove thrown just after switching back from the wrench;
  - the last bomb.
  - Partway through the throw, X jumps and R1 + X does the R1 jump.
  - Either can happen before the skid's lock would have allowed a jump.
- **Guns stop him.** A gun held while it fires (RC1's gun flag; RC2's equivalent is "hold") turns the skid into RC1's standing gun pose, with his slide and speed capped at 1.5 units/s, as RC2 does.
  - The guns: Blaster, Pyrocitor, Tesla Claw, Suck Cannon and Morph-O-Ray.
  - This happens in any skid, not only after a charge.
  - The skid's lock then keeps him planted until it runs out.
- **Gloves, the Devastator and the RYNO** leave him skidding with his speed. The RYNO counts as fired once, like RC2's RYNO II, not as a gun held while it fires; that is one word in RC1's weapon table.
- **Midair charge.** A Devastator or RYNO shot during a charge over a gap ends it into a skid in the air. R1 on the next frame starts a new charge before the skid falls, as in RC2.
  - The R1 press counts as a second tap only within 30 frames of an earlier press, for example the one that started the charge.
  - While the item in his hand is being switched, R1 presses don't count and the window stands still, as in RC2.
  - Gloves go into the standing throw instead, and guns stop him, as in RC2.
- **Re-charging.** Every R1 press opens the 30-frame double-tap window, including the press that started the charge.
  - If a weapon or the Magneboots end a charge within half a second, one more press starts a new one.
  - Otherwise it takes a normal double tap.
- **Sliding.** While Ratchet slides on, he slows at RC2's rates:

  | state | this mod (RC2), units/s² | RC1's own, units/s² |
  |---|---|---|
  | skid | 27 | 12 |
  | crouch | 27 | 6.48 |
  | standing | 17.7 | 12.6 |
  | glove throw | 35 | 35 (both games) |

  Standing slows at 17.7 always, not only after a charge (see Walking).

**Walking.** Ratchet walks as in RC2 (its walk handler, `src/c/walk.c`):
- **Speed follows the stick:** 5.95 units/s times the stick, where RC1 has only two speeds (0.9 below 0.82 of the stick, 5.7 above). He speeds up at 18 units/s² and slows at 20, instead of 7.5 and 8.5.
- **Moving along the stick:** below 1.7 units/s he moves along the stick's direction while he turns to face it. RC1 does that only from a standstill with the stick pushed past 0.85, and stops doing it once he runs, so he arcs round.
- **Turning:** RC2's turning, stiffer and better damped than RC1's, both when running and when walking, aiming a gun or in a quick turn.
- **Quick turn:** a reversal of the stick at full tilt (more than 110 degrees within six frames, with more than 70 degrees left to turn), or pushing it over 120 degrees away while nearly stopped. He then moves along the new direction at once, his speed is capped by how far he has left to turn, and he slows at 17 units/s². RC1 started its quick turn from a flick of the stick and kept moving along his facing.
- **Slow stick:** with the stick under 0.75, a turn of more than 35 degrees cuts his target speed.
- **Run jump:** a jump from the walk is the run jump whenever the stick is past 0.85 and he is not strafing. RC1 also wanted the run animation, the stick within 60 degrees of his facing and 85% of the run speed.
- **Run speed 5.95** is also the reference RC1 uses in the air and in the jump state, as RC2's is.

**In the air.** Two of RC2's numbers:
- **Braking:** with the stick let go during a jump, he brakes at 8 units/s² instead of RC1's 2, so he no longer drifts on. After touching a wall it stays 2, and in RC1's other case it is 11 instead of 6, as in RC2.
- **Air speed:** the stick asks for up to 5.95 units/s in every jump, as in the walk, instead of 5.7.

Jump heights, flips, long jumps and wall jumps are left as RC1's.

**Gloves.** Holding Circle keeps throwing, as in RC2: each throw comes as soon as the last one's animation is over. In RC1, a held Circle only throws within 17 frames of the glove coming out, so you have to tap.

**RYNO.** Weapons can be switched during its salvo: quick select, the pause menu, or Square to the wrench. RC1 locked Ratchet's hand for the whole salvo. Switching puts the RYNO away as any switch does: the missiles still to come are dropped, and those in flight carry on.

**Attacks.** While swinging the wrench, slamming or throwing a glove, he turns at up to RC2's 16.93 rad/s instead of 15.01.

**Stopping.** RC1's walk normally drops into a skid, with its skid animation, when you let go of the stick at speed (above 2.7 units/s). Below that, he walks on until the walk has lasted 25 frames. Here it works as RC2's walk:
- he goes straight to standing;
- his slide and speed are capped at 1.5 units/s;
- standing slows him at RC2's 17.7 units/s² (RC1's 12.6), so he stops within about five frames.

Skids from landings are unchanged; RC2 has those too.

**Camera.**
- The "Fast" camera rotation speed turns at RC2's 2.2 degrees per frame instead of 1.6. Slow and Medium are the same in both games.
- The camera follows Ratchet's movement as tightly as RC2's: factor 0.85 instead of 0.75.

**Strafing.** Hold L2 or R2 on foot.
- Ratchet faces the camera and moves along the stick. RC2's damping applies when left and right are reversed, and his body twists in a sideways strafe.
- Letting go of the stick goes straight to standing, without RC1's skid.
- A jump with the stick sideways or back is a side flip or back flip aimed along the strafe. In the air he keeps facing the camera.
- Walking backwards plays RC2's backwards walk.
- On foot, both buttons only strafe: in RC1, L2 is first-person view and R2 crouches, as R1 does.
- **At the start of a long jump R2 crouches again**, so runs that alternate R1 and R2 to chain long jumps and side flips still work.
  - It is RC1's crouch for the long jump's first 12 frames, while RC1 can cancel it into a side flip. After that it strafes again, so holding R2 out of a long jump strafes on landing.
  - L2 always strafes.
- There is no strafing during a charge or a long jump.

## Use

Enable the mod in RaCMAN **before loading a save**, and leave it on for the session. To turn it
off, or to switch to another version of it, disable it and restart the game.
- **Why:** the charge sound bank is registered with the game's sound library the first time you charge, inside the mod's own data.
- **What goes wrong:** disabling or re-enabling the mod mid-session rewrites that data under the game: the charge sound's definitions lose their bank and the bank its registration, while the mod still thinks it is loaded. The next time the game plays or resumes the charge sound (charging again, or unpausing), it crashes in the sound library (0x6098ac, reading a null bank).
- **Also:** RaCMAN writes the code while the game runs, and the mod's variables move between versions, so swapping versions mid-session is never safe.

Tested on a console (through v3.8). Not tested on RPCS3.

## What is left out

- The RC2 boots model on Ratchet's feet and the Foot Items menu entry. They need about 180 KB of data and heap space taken at boot.
- Box Breaker and Clank's dash.
- The charge is always available while the mod is on.
- From RC2's walking:
  - its body lean (RC2's lean function is a rewrite of RC1's; only animation);
  - a heading hold it keeps while one item (RC2 item 0x0A) is out;
  - its skid and crouch deceleration (27 units/s²) outside a charge: RC1 keeps 12 and 6.48 there, so landings and crouch slides (and long jumps out of them) are unchanged.

## Memory

| address | size | what |
|---|---|---|
| 0x6FD600 | 9584 of 10752 | code and constants, in the zero tail of the text segment's last 64 KB page (upstream uses the same cave) |
| 0x5394A0 | 2620 of 12588 | the walking (`src/c/walk.c`, `src/asm/walk_hooks.s`) and the item's stubs (`src/asm/item_hooks.s`), over RC1 code nothing reaches: its only callers (0x4ded28, 0x4defb0) are never called or referenced |
| 0x931400 | 43888 | upstream's two charge sound definitions, then its cut-down RC2 sound bank |
| 0x93C000 | 124 of 256 | the mod's variables, not written by the patch |
| 0x940000 | 19792 | RC2's charge animation (Ratchet animation 139), relocated for this address |
| 0x945000 | 38080 | RC2's backwards walk animation (Ratchet animation 0x14), relocated likewise |

The two animations go into slots 166 and 165 of Ratchet's model's animation table.
- **Where:** that table has 134 entries, followed by zeroed space up to the mesh in every level (slots 134 to 166), so the two slots are the last ones in that space.
- **Not slots 134 and 135:** RC1's scenes lend a model animations starting at the first free slot, and zero it again when they end, which happens on pausing.
- **When:** the slots are kept filled every frame, never over anything but zeroes.
- **Why every frame:** Ratchet's upper-body animation layers can come back to one of these animations after it has played.

0x931400 to 0x94A000 lies in a 128 KB bss table (0x9313A0–0x9513A0) that only dead code uses and
the game zeroes at boot.

The patch writes three data words for the ground snap's spring, and four for the camera: the speed table's Fast entry, and the follow factor's two constants plus one store. For the walking it writes three: the run speed in the gait table (5.95), and the ground acceleration in the hero table (18 and 20), which five other ground states read too, as in RC2. One more data word makes the RYNO count as fired once, and two let weapons be switched during its salvo. Two set the Charge Boots' price to 1,000 bolts, in the item price table (0x737FE8), and two take the crouch out of look mode (0xBFE90, 0xC7FC4).

It also writes 46 words behind the data, the hooks:
- **The item (2):** `src/asm/item_hooks.s`: the text lookup and challenge mode.
- **Charge (15):** `src/asm/charge_hooks.s`.
  - frame, update, weapon chooser and transitions;
  - skid, and the skid, crouch and idle deceleration;
  - glove throw and glove hold, the throw converter and the Devastator.
- **Movement (6):** `src/asm/move_hooks.s`. Stopping (3), the air brake, and the attack turn speed (2).
- **Walking (9):** `src/asm/walk_hooks.s`.
  - three in the walk handler;
  - two in the walk transitions: the run jump, and a `nop` over RC1's quick turn;
  - state 0x20's speed threshold;
  - three data words: the run speed and the ground acceleration.
- **Strafing (14):** `src/asm/strafe_hooks.s` and `src/asm/optional/back_walk.s`.

Other RC1 mods in this folder use the 0x4F5xxx–0x4F9xxx cave, so they can run alongside.

## Rebuilding

`make` builds `bin/rack.bin`, `bin/rack2.bin` (the second cave) and `patch.txt` from `src/`, and
prints how much of the two code caves and the variables it uses. Run it in WSL or Linux, with the `powerpc64-linux-gnu` binutils and gcc.
`make data` regenerates `data/` from the vendored upstream assets (`tools/make_data.py`).

**Where things are:**
- The charge's numbers are at the top of `src/c/charge.c`.
- Strafing's numbers are at the top of `src/c/strafe.c`.
- The walking's numbers are at the top of `src/c/walk.c`.
- The strafe buttons are `STRAFE_BUTTONS` in `src/c/strafe.c`.

**Strafing switch.** `BACK_ANIM_ADDR` is a `#define` at the top of `src/c/strafe.c`.
- The Makefile reads it, so editing the line and running `make` is all it takes; `make BACK_ANIM_ADDR=value` overrides it for a single build.
- It is where the patch writes RC2's backwards walk animation (default 0x945000). 0 drops the hook `src/asm/optional/back_walk.s`, and walking backwards plays RC1's own walk cycle. The data line in `src/patch.txt` stays.
- The old switch `RC2_ACCELERATION` is gone: RC2's acceleration is part of the walking, always on.

## Licence

Upstream's code is under the Apache License 2.0 (`tools/vendor/RAC-1-Mods/LICENSE`), and the code here is ported from
it. The animation and sound data come from Ratchet & Clank 2 and belong to their owners.
