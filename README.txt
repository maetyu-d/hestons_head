AFTERTASTE / MOLECULAR WORKS 3.27.0
Invent dishes on the actual factory floor / native C++17

Integrated demo campus
------------------------
Demo rebuilds the showcase as one balanced campus on the same white floor:
- North: six complete growing/harvest/wash/grade/chill lanes. Tomato produce
  travels directly down the receiving aisle into the cooking hall. Other crops
  become named farm stock for actual measured feeds.
- Northwest: compact solar, wind and sealed experimental nuclear benches share
  chassis contacts; a single AC spine distributes their measured output.
- West: water collection/treatment/reuse and four reagent lines supply the farm
  and kitchen. Pectin, nutrients, xanthan and yolk-derived lecithin are made from
  real feeds. Organic extraction residue goes to a biogas train, whose digestate
  feeds a nursery composter. Cooling and recovered engine heat reach the kitchen.
- Centre: tomato emulsion separates a side fraction, cultures/aerates it, then
  recombines it at a plater. A carrot/cream line and a glass/lattice research lane
  share the services. Measurement instruments sit beside the actual processes.
- East: tasting docks outside the cooking hall. South: three separate distillate
  receivers occupy valid wall openings. White aisles and spare floor allow edits.

The demo is staged with 48 real simulation ticks and opens paused. Feedstocks,
seeds, experimental cartridges and reserves are finite; filter backwashing and
resource balancing remain part of play. Eight named process plans preserve the
new districts and their settings/routes. Farm, Power, Water and Chem centre the
same floor; Fit reserves space for the legend and includes the whole campus.

Demo saves the previous factory to a timestamped factory-before-demo JSON and
sets it as the Load checkpoint. Any earlier checkpoint also gets a timestamped
backup. Existing saved worlds are never automatically rearranged. New installs
and --fresh-factory use this campus; legacy fixtures/migrations remain supported.

Reading production flows
------------------------
Water and electricity flow markers draw above equipment, including edge-to-edge
bench connections. Port chevrons show input/output direction even when paused.
Larger repeated water droplets and square electrical pulses show actual transfer;
solid arrows show its measured direction. Isolated/tripped routes have a red X.
Water treatment streams and heat/steam conversion routes are visible by default.
Cable/pipe inspectors distinguish flowing, idle and isolated states; bench
contacts list measured incoming/outgoing transfers by medium and rate.

Bright primary palette
------------------------
The whole game uses a brighter, smaller colour vocabulary: bold red, blue,
yellow and green machine paint, with violet for chemical details and cyan for
glass and cold processes. Machine bodies and accents have stronger colour;
metal, glass and shadows keep the actual material windows readable.

Panels and the floor use white backgrounds with a quiet neutral grid. Factory
zoning uses a green boundary instead of a floor tint. Text/outlines stay deep navy. Active controls are primary blue and
main actions red. Ingredient chips, crop art, food mixtures, previews and the
minimap share the same display palette. Dark food pigments retain a darker
shade, and browning, scorching and crop damage remain visible.

Resource routing keeps its identities: water blue, chemicals violet and
electricity yellow/gold. This is a display change; saved ingredients, machine
settings, molecular state and all factory resources retain their exact values.


Reading water, chemical and electricity routes
----------------------------------------------
Water uses saturated blue pipework with round flanges, reflective interiors
and measured moving droplets. Raw, clean, greywater and reject pipes use
related shades of blue with independent connections. Boiler return water
and cooling circuits share the pipe silhouette.

Chemicals use visibly violet glass walls with twin violet collars. The material inside keeps
its actual ingredient colour. This applies to the reagent lab's main and side
streams and to connected glass tubing, including empty tubing. Side outlets
keep violet collars with small amber direction marks. Assay instruments use dotted measurement leads.

Electricity uses visibly gold insulated trays, bright gold AC or amber DC,
square clips and small plugs. All cable corners stay square. Crossings have
an insulating gap; matching junctions have square connection boxes. Lab AC
contact plugs sit below the glass material path. Water contacts are round.
The floor key includes a drawn example of each family.

Only real material ports draw material connections. Water/power apparatus and
side receivers no longer display hypothetical food pipes between their bodies.
The existing separately typed networks, process settings and save format 18
continue to supply the same factory.


Reagent laboratory / the fourth supply system
---------------------------------------------
Select Chem in the floor toolbar. Three parallel material lines below the
waterworks make the last core factory input: named chemicals. All eleven new
apparatus types are buildable, and the complete 7 by 8 arrangement is in Plans.
There is one floor, one clock, and actual batches between every process.

Crop feeder -> extraction -> separation -> polishing -> concentration -> flask
                                                            assay -> inlet
Mineral hopper -> reaction bench -> the same purification/measurement stages
Crop feeder -> culture vat -> the same purification/measurement stages

Feed crop batches directly from harvest and washing, or use pantry feeders that
draw the existing named farm stock. Extraction recovers actual pectin, starch,
lecithin or protein precursor; extracted precursor is removed from the residue.
Cultures use actual fermentable crop sugars, a small finite starter, water and
AC. Choose culture, yeast, xanthan, amylase, binder or lactic acid. Temperature,
dwell, crop condition and contamination affect yield and the purification load.
These are stylised game conversions, not real chemical manufacturing recipes.

Mineral refining makes farm nutrients, salt, calcium, citrate or bicarbonate.
Import raw mineral blend at the hopper (+500 g; 20 credits in Economy), then
refine it on the floor. Calcium and citrate additionally consume an actual acid
reagent supply, so fermentation and mineral refining can form a process chain.
Load a starter at a culture vat (+10 g; 8 credits in Economy). Once the lab makes
its own culture, use 1 g from named stock to seed the vat again. Raw feedstock
and initial starter imports remain the boundary to systems not built yet.

Separators trade recovery for dry purity. Polishing removes most remaining
impurities at a small active loss. The concentrator removes carrier water into
an actual side condensate; overheating damages active reagent into impurity.
Both product and side fraction remain material, with mass conserved. Side
streams can feed another purification/extraction, compost or organic power
processes as appropriate. Receivers return carrier water to the waterworks
for treatment and hold finite solids; haul these to restore capacity.

A flask holds its actual lot until a powered adjacent assay instrument points
at it. The instrument certifies that batch identity and reports dry purity and
concentration. The factory inlet requires an assay, >=85% dry purity and >=35%
concentration. An off-spec lot stays held and can leave the side port for further
processing. Only the named active ingredient is added to pantry/factory stock.
Making pectin does not fund calcium, enzymes or any other unrelated reagent.
Fertigation consumes mineral nutrients, closing the farm/lab supply loop.

Chemical quantities are grams of the corresponding pantry formulation. Dry
purity means formulation versus unwanted impurity; concentration includes the
carrier. Factory CHEM totals include named stock plus any generic imported
reserve. Generic deliveries remain an optional fallback. Sealed fictional
nuclear cartridges still use their own imported feed rather than edible agents.

Lab chassis share AC by contact; a single square header and perimeter branch
connect the demo to the factory grid. Glass tubing and conveyors move real lots.
Distinct flasks, vats, spinning separators, columns and assay equipment show
actual material on the floor. The inspector exposes recipes, dwell, temperature,
selectivity, lot measurements, side fractions and named inventory.

Plans retain settings, directions and material/electrical routes, and copy no
feedstock, starter, live lots, retained solids or measurements. Save format 18
retains existing factories and adds the empty lab only on a vacant site. It does
not advance time or grant chemicals, minerals or cultures to existing saves.


Waterworks / collection, treatment and reuse
--------------------------------------------
Select Water in the floor toolbar. The compact plant below the power rack has
12 distinct facilities and six typed water pipe streams. Its main process is:

Borehole + rain -> raw buffer -> sand filter -> membranes -> UV -> clean tank
                                                              -> factory inlet
Process-water return -> reed reclamation -> raw buffer; reject -> waste tank

The borehole spends real AC and draws from a shared finite 2,000 L aquifer.
Rain is intermittent, adds less salt, and slowly recharges groundwater. Actual
water parcels carry suspended solids, dissolved salts and biological load.
Sand captures solids in finite media; membranes split permeate and concentrate;
UV spends electricity reducing the biological load. Higher membrane recovery
saves water but increases salt passage. Fouling reduces flow and selectivity.

The inlet tests actual quality before adding litres to the existing farm/factory
input buffer. Dirty clean-labelled water is held, with the failing readings
visible in its inspector. The default buffer target is 180 L. Sources, storage,
filters and the inlet all stop correctly when their downstream capacity fills.
No disconnected or unpowered treatment stage manufactures clean water.

A powered backwash spends 2 L of actual clean stock and moves captured solids
to concentrate. Purge unsafe stored water through the reject line. Haul actual
concentrate from its finite tank to restore capacity; Economy charges 0.02 per L.
Process washing returns a measured fraction to greywater. Irrigation and boiler
makeup are not captured as sewer returns. Reed reclamation recovers 90% as raw
water, which still passes through the complete treatment line.

Point the quality monitor at water equipment. Inspect volume and loads per litre,
flow, aquifer level, filter loading, recovery and UV dose. The floor shows actual
flow, filtration silt, membrane yield and post-UV biology; distinct machine
shapes and coloured ports keep all processing in the same factory view.
Matching reservoirs join by contact as a bank. Different water streams remain
separate even when their pipes cross. The waterworks uses one short AC branch.

Water quality and UV dose are stylised game values. The displayed game limits
are solids 0.15, salt 1 and biological load 0.02 per L. Reusable plans preserve
settings, directions and routes while clearing stored water, filter cake and
observations. Save format 17 adds the empty demo equipment without advancing
the existing factory or granting clean water. Emergency direct water deliveries
remain available through Factory Inputs.


Contact power rack / one factory feed
-------------------------------------
The same 44 power and laboratory devices now fit a 10 by 7 rack. Electrical
collection happens through touching chassis, so the rack needs no internal
power cables or perimeter collector. One straight two-cell AC lead reaches the
existing factory trunk. The output switchboard is the single electrical exit;
opening it disconnects their AC supply. No parallel feed bypasses it.

Return water uses the actual chassis manifold. The feed pump introduces finite
supplied water, the boiler consumes it, and the condenser returns working fluid.
Steam, exhaust, gas and shafts dock directly at their matching process ports.
Heat remains a separate measured process: short recovery pipes feed the
exchanger and tank. A top tap on the tank and the cooler's contact manifold
export heat and cooling alongside the same straight factory service corridor.
Water, heat, cooling, AC and DC remain separate media with real source budgets.

Power frames the complete rack above the floor legend. Lab and Solar focus
individual experiments in the same workspace. Visible couplers show actual
contact transfers; isolated or missing equipment opens those connections.

Save format 16 moves only the unchanged stock campus and its standard routes.
It retains all live material, fuel, charge, wear, readings, records, input buffers
and clock. Only the condenser's sockets turn to dock into the turbine exhaust.
Custom construction, cable ratings, route masks, isolation and directions block
automatic rearrangement. No fuel or stored energy is granted during relocation.


Solar banks and docked generator trains
---------------------------------------
Use Solar in the floor toolbar to focus the array. Panels join on every touching
cardinal edge and combine their DC output in rows or blocks, regardless of panel
rotation. Dock battery racks and an inverter directly. Panel couplers carry DC
only, so they do not accidentally bridge AC, cooling, vacuum or control services.
A gap, isolated panel or diagonal placement breaks that connection.

The demo solar bank now places its existing three panels, battery, inverter and
reserve relay together. The wind rotor, gearbox and alternator form a direct
shaft train. Boiler, steam accumulator, turbine and alternator dock directly;
the gas scrubber, holder, engine, flywheel and alternator do the same. Output AC
still travels by cable to the factory. Separated return-water, exhaust and heat
recovery loops retain their typed pipes. Matching process couplings show actual
flow pulses, just like the longer conduits.

Version 13 stock yards are rearranged on load with every machine's settings,
material, stored energy, wear and readings retained. No extra panels, fuel or
charge are granted. Custom construction, routes, ratings, isolation or orientation
prevent an automatic rearrangement. Stock construction plans are refreshed;
custom plans remain as authored. Power frames the conventional yard, Lab frames
the nuclear apparatus, and Solar frames the array within the same floor view.


Docked power apparatus
----------------------
Place power equipment edge to edge. Its chassis couplers share separate AC,
DC, cooling, vacuum and control services with adjoining power equipment.
Only the four cardinal edges connect: a gap or diagonal placement breaks the
contact. Isolating or tripping a machine opens its couplers. No energy is created
by a contact; providers still consume actual stored energy, fuel, water and work.
Heat, shaft, steam and other process stages dock through matching physical ports.

The nuclear demonstration is now a compact 10-piece bench, with no local service
cables or pipes. Its reactor and star chamber touch their calorimeters; the
recorder, pump, bath, battery and inverter share bench contacts. One AC cable
leaves the switchboard and joins the factory supply. A broken export cable or
open switchboard cuts off remote consumers while the local experiments remain
assembled. Conduits remain available when you deliberately separate equipment.

Short connector cassettes show occupied chassis edges. Their coloured pins
identify separate services and flash only during actual transfers. The inspector
lists docked neighbours and distinguishes bench service from external ports.
The Lab toolbar button frames this compact assembly at a useful working scale.

Stock version 12 nuclear layouts are compacted on load without changing any
machine state, fuel, readings, resources, factory food or time. Custom apparatus,
orientations, service ratings, isolation or construction block automatic moving.
Saved stock process plans receive the new compact arrangement; custom plans stay
as authored. Rotated and stamped contact benches retain their working topology.


Curiosity laboratory / experimental nuclear power
-------------------------------------------------
Select Lab in the floor toolbar to focus the new bench west of the old power
installation. A sealed curiosity pile and a jam-jar star chamber use fictional
oddium cartridges. Bell jars, electrode cages, glass cold traps, an immersed-coil
quench bath, twin-bulb calorimeters and an oscilloscope make each stage visible.
All nuclear quantities are invented game units; tuning is an experimental
puzzle. Existing solar, wind and residue generation remain available as support.

Adjoining recorder -> bench contacts -> chamber interlock.
Adjoining pump -> vacuum coupler -> star chamber.
Water + AC -> quench bath -> chamber and calorimeter cold jackets.
Cartridge -> experimental heat -> calorimeter -> DC -> inverter -> factory AC.

Arm or scram each chamber, adjust resonance and drive, and vary star-chamber
pulse timing. The star draws real excitation power. Tuning, vacuum and cooling
change measured yield; a poor experiment can recover less DC than it spends.
A detached or isolated recorder opens the interlock. Lost vacuum stops pulses. A hot
jacket latches a scram. Scram removes drive while residual excitation decays.
Disarm and cool before changing cartridges; each new 40-charge cartridge uses
0.40 kg from the supplied chemical buffer. The quench bath uses supplied water;
both relevant input delivery controls are available in the lab inspectors.

The recorder saves up to 16 runs, each covering 24 powered ticks: heat produced,
electrical draw, recovered DC, stability, and the actual resonance/drive/pulse
settings. Net return includes chamber, vacuum pump, bath and recorder electricity;
inverter and cable losses occur downstream. Compare runs after changing one
setting. Switch other chambers off to investigate a single experiment.

The reusable Curiosity laboratory plan keeps equipment, tuning, orientation and
all material/service routes. It does not copy cartridge charge, stored energy,
armed state or observations. Old saves receive a connected annex only where its
site is vacant. Existing machines and food are retained; two demonstration
cartridges are added, and up to 40 existing imported electricity units are moved
into the lab battery for startup. Reloading cannot grant the samples again.


Conduit appearance
-------------------
Straight cable runs align with their machine terminals along the whole run.
Fixed offsets no longer create small staircase bends at the ends. Cables use
muted casing, a pale inset and a restrained conductor, with short connector
sleeves and square junction fittings. Live flow appears as small travelling
packets inside the conduit. Shaft couplers remain distinct from electrical
cables. Focused service views hide other networks completely; All services
shows the complete installation. The demo's auxiliary bus clears the flywheel.
Default version 10 demo wiring upgrades that one bus without moving machines
or changing food, crops, settings, resource stores or time. Custom routes stay
as built.

Clear service routing
----------------------
The demo uses a single AC collector in the western yard, with short generator
and auxiliary branches. One service corridor runs beside the farm and kitchen;
row branches replace the earlier mesh of individual risers. Steam, gas, water,
heat and cooling have deliberate routes. Parallel runs remain separate unless
they share an intended junction; process-plan rotation retains those connections.

Select power equipment or a conduit to isolate its services and hide the rest.
Routes / all services in the inspector restores the complete utility drawing.
Thinner lines meet the actual coloured terminals through square, right-angle
bends. Flow pulses follow those same bends, and only
real junctions have dots. Flow pulses remain tied to actual material transfers.

Version 9 demos with their original generated wiring receive the cleaner routes
on load. Equipment positions, settings, food, crops, energy stores and the clock
remain unchanged. Custom cables, ratings, isolation settings and changed device
directions are preserved instead of being replaced automatically.

Electricity production
----------------------
The western power district shares the same floor and clock with the northern
farm and molecular works. Use Power in the floor toolbar to centre it, or Fit
to see all three districts. Solar and wind respond to the shared day, season
and gusts; walls and tall equipment reduce exposure. Panels produce DC, wind
rotors supply shafts, alternators produce AC, and inverters connect DC storage
to the AC factory. Gear ratio changes shaft speed and alternator efficiency.

Build AC/DC cables, drive shafts, live/exhaust steam pipes, raw/clean gas lines,
return-water pipes and recovered heat/cooling circuits by dragging. These
layers share floor cells with food equipment. Different media cross without
joining; matching media form junctions. The small coloured machine terminals
show actual input/output directions. Rotate equipment to change its terminals.
Right-click selects a route, or choose Inspect energy routes below in a machine
inspector. A conduit brush selects its own layer beneath equipment. Cycle Layer
to inspect overlapping routes, change ratings, isolate or reset them.

The biomass train burns actual organic food batches. Ignition needs a small AC
black-start supply. Steam production needs water; the turbine returns equal
exhaust mass, and its condenser recovers boiler water and useful heat. A feed
pump adds measured makeup water. Wet fuel reduces heat yield. Ash leaves the
boiler's clockwise food side outlet; blocked residues stop new firing.

The digester consumes actual organic compounds from incoming leftovers. Warmth
changes conversion speed. Raw biogas needs a scrubber with connected water;
clean gas can be stored or consumed by an engine. Engine starting uses AC,
running consumes gas, and its shaft drives an alternator. Digestate is retained
as a real side stream. Both residues can supply a compost bin beside a grow bed;
industrial residues cannot be served or recorded as edible dishes.

Heat exchangers, heat tanks and recovery cooling plants reuse generation heat.
Heaters and chillers can replace part of their AC demand with connected thermal
supply. Tanks have a controllable excess-heat bypass; closing every heat outlet
can back up an engine or condenser. Every store is finite, every conduit has a
throughput rating, and conversions lose energy. Batteries, flywheels and steam
accumulators have independent capacity, output-rate and standing-loss limits.
A flywheel can be hand-cranked for an explicit small external work input.

The reserve relay uses separate low/start and high/stop battery thresholds to
avoid rapid switching. Set boilers and gas engines to reserve or continuous
operation. Food and farm loads have Critical, Normal and Optional priorities.
Under shortage, mixers and heaters advance more slowly on partial supply;
precision processes hold. Overloaded electrical conductors and switchboards
accumulate heat and trip. Isolate, let them cool, then reset the breaker.

The inspector shows actual flows, RPM, stores, resource ports and measured output
history. Flow animates only committed transfers and freezes when paused. Plans
save the complete arrangement and typed routes but never duplicate fuel, stored
energy or live food. Version 8 northern demo saves receive the western district
when its site is clear; existing machines, crops, food and other buffers remain
intact. Existing electricity is moved into the initial battery reserve up to
250 units, never created. The optional metered grid inlet is initially isolated;
when enabled, it draws a limited rate from delivered import reserves.

Serving and feedback
--------------------
Each tasting dock can be set to Restaurant pass, Research tasting or Chilled
pickup. Restaurant pass checks the active optional request when the actual
batch arrives: material phase, required compound masses, temperature window
and quality. Matching servings advance a streak and the ticket expires after
its displayed simulation-tick window; expiry resets the streak without losing
food. A missed target lists the criteria met, the misses and practical process
changes to try.

Research tasting consumes a sample and records measured composition and process
notes without sales value or ticket pressure. Chilled pickup accepts only a
batch at 8 C or colder; a warm batch stays held until it is chilled. Each dock
retains its last serving report, and Recent Service in the inspector keeps a
short history across all serve points. All audiences operate on real floor
batches; serving consumes the batch while its last-output record stays read-only.

Connected factory systems
-------------------------
Food ports are directional. Cyan marks inputs, dark teal marks the main outlet,
and orange marks a clockwise side stream. The inspector lists the current
neighbours at every port. Separators retain both outputs as real batches, so
route a useful residue or fraction into another machine instead of discarding it.

Utility stations and matching utility lines create shared power, heat, cooling,
water and vacuum networks. Connected lines share station capacity, and adjacent
machines tap the service they use. A station activates that service network;
processes that need it require an adjacent line and enough shared capacity.
Without a station, the added network constraint is off. Without production equipment, the legacy electricity buffer supplies loads.
With a physical grid, connected generation supplies them and a metered grid inlet
is needed to use imported electricity. Process water still depletes in every mode;
water draw follows each machine's water demand. These buffers are separate from
optional utility-line capacity. Economy also applies its grid-capacity limit and
operating fees. Heating and cooling demand follows the machine's selected
temperature target.

Floor farming
-------------
The demo's six farm outputs feed two physical conveyor lines into the molecular
factory. Every crop keeps its original batch identity, mass, condition and bed
provenance along that route. Produce intakes have Store crop and Send to factory
modes; Send passes actual food onward without crediting shared stock. Blocked
outlets hold the crop. The demo starts with staggered planted crops so deliveries
begin promptly. Existing northern demo farms receive these connections when
the aisles are clear; straight conveyor openings retain the factory zoning.

Farming shares the factory floor and simulation clock. The Flow overlay shows
seed planting, nutrient delivery, working grow lights and pollination along
their actual bed connections. Growing crops use their variety colour, growth
stage, health and moisture readings. Harvested crops remain visible as produce
on conveyors and inside harvesters, washers, graders and cold stores. Soil
speckles disappear as washing removes dirt; grade, freshness and temperature
readouts follow the real batch. The intake pulses when it stores a batch and
keeps the last received crop as a read-only record. Route the crop conveyor
straight into factory processing to use the batch directly, or send it through
an intake to supply named farm inventory.

The starter demo places
a six-crop farm north of the molecular works: fruiting crops, leafy greens,
roots, peppers, herbs and shade-grown mushrooms each occupy a separate bed and
harvest lane. Raised soil, field plots and hydroponic troughs trade speed,
moisture buffering and exposure to outdoor seasons.

Spring, summer, autumn and winter shift outdoor bed temperature and daylight.
Hydroponic beds buffer seasonal temperature, and light rigs can spend factory
power to extend the growing window. Crop-specific temperature, water, pH, light
and NPK preferences affect growth, health, yield and the real ingredient batch.
Soil probes report the live bed readings.

Harvest records the last crop family in each bed. Repeating fruit, leaf, root
or fungal crops raises rotation pressure, reducing growth, health and grade.
Switching crop families eases the pressure; compost returns organic matter and
nutrients while helping restore tired soil. Seed hoppers plant from finite
variety-specific stocks, so players can change a hopper crop to rotate a whole
row or plant beds directly from the inspector.

Fertigation mixers spend shared water and chemicals to replenish moisture and
NPK. Pollinator hives improve fruit set. Harvesters make real crop batches that
reflect the growing conditions; route each through its dedicated washer, grader
and cold store into a named produce intake. Stored crops feed matching factory
feeders. Power, water, chemicals, seeds and cold-chain time constrain the farm
alongside the molecular processes.

Place a pH probe, temperature logger, droplet microscope or aroma sensor beside
a batch and rotate it toward the sample. Each tool reports its measured value,
batch identity and recent history. Use the readings to follow changes along a
process and locate the step that changed a batch.

Open Plans to capture a named floor area. Include every branch and utility line
in the selection to preserve machine settings, port directions, side routes and
service topology. Stamp the plan to reproduce the arrangement; it never copies
food.

Open Aftertaste.app on this Apple Silicon Mac, or Start Aftertaste.command.
Quit any older running copy first. There is one persistent factory view.

Make a dish with real machines
------------------------------
There is no separate test plate or instant processing interface. Ingredients enter
through feeders and dosers. Mixing, aeration, heating, freezing, fermentation,
separation and plating happen in the actual machines on the floor, using the
shared simulation clock. The centre of the screen is entirely factory floor.
The selected machine's live batch and controls stay in the right inspector. Flow
hover details are reserved for other machines, keeping the selected equipment
visible. Quality, fraction phase, machine state and material-flow symbols use
explicit labels.

The lower-left experiment cell is ready to use:

                      feeder B
                         |
                       conveyor
                         |
  feeder A -> conveyor -> mixer -> doser -> lattice reactor -> shared line

1. The starter demo begins with 100 g tomato juice and 30 g olive oil staged in
   its two real feeders. Press Space or Step to move them through the mixer and
   1.5 g lecithin doser. The lattice reactor holds the resulting batch.
2. Select feeder A at -7, 6 or feeder B at -5, 4 to change its ingredient. Choose
   an ingredient from the pantry and Set source. Click the gram value to type an
   exact dose, then Enter or Set source. After a feeder empties, Dispense once
   loads its next measured batch.
3. Select the lattice reactor at -3, 6. The right-hand preview board is built
   from compound families actually present in the batch.
4. Swap adjacent tokens in that preview to match three. A valid match
   queues one real chemistry step. Step or Run applies it, using the batch's
   current compounds and the reactor's temperature and shear. Repeat, or choose
   Release batch to send it into the connected factory line.
5. The released batch joins the shared production network. A separate Cell
   placed from the build library ends at a holding tasting dock after the reactor;
   inspect its composition, reactions, compound masses and history there, then
   name it and Record dish.
6. Build line selects a construction plan containing the actual contributing
   feeders, conveyors and processing machines. Stamp it elsewhere on the floor.
   No food is copied. The new feeders and final dock run automatically, consuming
   ingredients and producing the dish through the same physical stages.

The Cell button places another real experiment cell anywhere. Its feeders are
single-dose and empty; its final dock holds output for inspection. Alter the real
equipment, connect extra stages, adjust ratios/temperature/shear, or build your
own line from scratch. There is no predefined recipe tree or ending.

Physical experimentation
------------------------
Single-batch mode waits for Dispense once. Auto feed continuously supplies the
selected ingredient at its measured dose. Empty single-mode feeders do not spawn
material merely because time advances. Finite economy charges ingredients when
they actually enter a feeder or doser.

A mixer needs two batches. A component plater joins two inputs while keeping their
components and texture contrast separate. Separation happens in a centrifuge or
filter. Route its clockwise side outlet so retained by-products do not back up
the line. Every transported batch moves at most one tile per simulation tick.

Factory walls
-------------
Search for Factory wall in the build library, then drag wall tiles around a
perimeter. A closed loop marks its enclosed floor with a soft green tint as a
factory area; untinted open floor remains outside. Wall tiles occupy floor cells
and stop batches from routing through them. Place any number of tasting docks,
receiving beakers or fraction receivers outside the enclosed area. A serve point
or conveyor can occupy a straight one-tile opening in the perimeter and still
count as its boundary; the opening tile stays outside the tinted factory floor.
The game prevents wall or conveyor placement from enclosing a serve point. Remove
a wall segment to open the area.

Molecular lattice reactor
-------------------------
The reactor's 5 by 5 board is a control surface for the batch it physically holds.
Each coloured token represents a compound family found in that batch; it does not
add or remove ingredients. Swap two adjacent occupied cells to align three or more
tokens. A valid move queues one chemistry operation, and the shared Step/Run clock
applies that operation to the real food and records it in process history. The
board then rebuilds from the changed compounds. A queued reaction can be released
after it resolves; with no queued reaction, Release batch sends the food onward.

Sugar and starch matches use heat. Protein matches aerate; fat and lecithin matches
homogenise. Hydrocolloid matches set or crosslink actual gels, calcium matches form
alginate shells, and acid/base matches neutralise. The inspector's temperature and
shear controls affect reactions, while missing partners or conditions explain why
a match cannot resolve. Add ingredients or change the actual machine conditions to
open more families and reactions. No score or abstract match result substitutes
for molecular state change.

Tasting docks have three modes:
  Hold: keep the actual batch for tasting, naming or further experimentation.
  Auto serve: serve each arriving batch and record its actual quality/value.
  Forward: send incoming batches into the next connected machine.
Send onward queues one held batch for the next simulation tick. Connect another
processor to the dock's outlet to rework a result. Serve batch consumes real held
food. Reclaim batch counts discarded material as waste; ingredients are not refunded.

Served docks retain their latest result as a read-only tasting record. It does
not age, occupy a production slot, or become usable inventory. Opening a recorded
dish likewise cannot add ingredients, transform material or spawn finished food.

Dish records and physical production lines
-----------------------------------------
Record dish is available at a tasting dock with an actual held batch or a genuine
last serving. It stores that result's measured composition, process history and
quality, together with its contributing physical machines and existing side routes.
Recording does not consume, move or duplicate the batch.

Build line stamps machinery with its feeder doses, dwell, temperature, shear and
routing. The final dock retains the recorded dish name. Intermediate tasting docks
that the batch passed through forward material instead of serving it too early.
The construction plan contains no active or archived food. Normal stock, power,
water, congestion, cleaning and timing still apply to the new production line.

A recorded result is a reference, not a guarantee that every later batch will be
identical. Jammed routes, changed settings and different holds can change what
actual machines produce. Pause before editing settings if you want a controlled
comparison. The recipe book supports 500 records and batches up to 16 components.
Plans capture current machine settings; distributed lines spanning over 1000 tiles
can be recorded as references, with smaller sections copied as ordinary plans.

Build a glass laboratory
------------------------
Search glass in the build library, or choose Plans > Glass lab apparatus. Place
its real 23-device construction plan on empty floor. Both feeder branches have
equal length and start in single-dose mode: 100 g tomato and 30 g olive oil.
Dispense each feed and Run/Step. The Y-junction waits for both actual batches;
the burette adds 1.5 g lecithin, the flask heats, the condenser cools, the residence
coil holds, and the receiving beaker keeps the actual output for inspection.

Glass tubing: drag to paint a run, as with conveyors. Turns follow connected
upstream outlets and the chosen output direction; R rotates that outlet. Ground
glass joints and transparent walls remain visible even with the Flow overlay off.
Actual food appears as coloured slugs inside the tube. Movement follows successful
physical transfers; stalled lines retain their food without invented circulation.
Tube traffic remains one batch per tile, at most one tile per simulation tick.
Separate batches share a line sequentially; use a Y-junction to actually blend them.

Glass residence coil: holds a batch for its configured dwell while ambient food
chemistry continues. Its visible contents advance with the real processing ticks.
Glass stopcock: select it and Close valve or Open valve. A closed stopcock blocks
incoming batches and retains any batch already inside; reopening resumes the same
material. Existing backpressure, disabled equipment and economy rules still apply.

Receiving beaker: graduated glassware with Hold, Auto serve and Forward modes.
It supports tasting and recording an actual dish, serving real held food, or
sending the batch onward through connected apparatus. A served result is read-only.
Heated reaction flask: a round-bottom vessel on a mantle; configure its target
and dwell to change the actual food's temperature and thermal chemistry.
Cooling condenser: a jacketed heat exchanger that cools the existing food stream,
using utility water. It also condenses real vapour from a fractional distillation
train; ordinary liquid batches still follow the normal cooling and freezing model.
Glass burette: choose its ingredient and exact grams, then Set source. It adds
that measured amount once per incoming batch, with actual pantry consumption.
Glass mixing junction: a transparent two-input mixer with configurable shear and
dwell. Both ingredient quantities and their process histories remain in the result.

Glass apparatus can connect to all existing processors, separators, feeders and
docks. Record dish at a beaker captures the actual contributing tube routes,
settings and glassware. Build line copies that equipment without copying food;
its feeders and final beaker become automatic. The equal-length feed branches in
the supplied plan support repeated production through the same physical stages.
All glass types, batches, valve states, directions, recipes and blueprints save.
Older factories retain their original machines and coordinates when opened.
The world remains a single persistent factory floor with room to expand.

Fractional distillation and collection
--------------------------------------
Choose Plans > Fractional distillation train to place a complete 26-machine line
on the factory floor. It has one measured feeder, three heated stills, three
reflux columns, glass vapour lines, three cooling condensers and separate
fraction receivers, plus a stillage receiver for the final residue. It is a
normal editable construction plan; move, extend or reconfigure its real devices.

The first still sends the selected heads cut through its clockwise side outlet.
Its main outlet keeps the concentrated stillage moving to the hearts still, and
then to the tails still. Each still has its own Heads / Hearts / Tails control
and target temperature. The supplied line uses a different cut at each stage.
Changing the source to wine or fermenting a sugar feed first supplies ethanol;
citrus and other ingredients contribute water, acid and volatile aroma.

Each side stream enters a reflux column. The adjustable theoretical stage count
changes how much ethanol, water, acid and aroma reports to the enriched vapour
or returns as water-rich liquid reflux. That reflux travels through the column's
clockwise side port back to the same still, where it can be boiled again. The
column's main outlet carries the enriched vapour to its condenser. Condensing
changes that same batch to liquid and preserves its named cut; it does not create
extra mass. Heads, hearts and tails collect in their own glass receivers, where
each can be inspected, recorded, served, or forwarded for more processing.

The split conserves each compound between the main and side streams. Live floor
labels identify vapour, cut and liquid state; selecting a machine shows its
composition, process events and output status. Cut, temperature and stage
settings are stored with recorded dishes and copied production lines.

Recognise machines by their equipment
-------------------------------------
All 38 physical machines have distinct equipment drawings and mechanisms; the
legacy compatibility kitchen is an additional, non-buildable device kind.
The same vector artwork appears on the floor, in the machine library and in
translucent construction previews. Equipment and its food windows rotate together
with the actual outlet, including the clockwise side ports of separators.

Feeders have tapered hoppers; mixers have paddle bowls; precision dosers have
calibrated syringes; homogenisers have pressure pistons; spherification baths have
perforated dipping heads. Aerators have sparger tanks with air nozzles, and setting cabinets have gel racks.
Chillers have cooling chests and snowflakes; heaters
have burner pots; dehydrators have racks and fans. Serving docks have plates and
cutlery, while component platers have two assembly arms around a plate.

Other devices have their own physical cues: a glass aroma bell, a hot/cold thermal
cycler, an immersion circulator and pouch for sous vide, a conical fermentation
vat with airlock, enzyme scissors, a centrifuge rotor with tubes, filter mesh and
fraction outlets, a bolted vacuum chamber with pump, cold vacuum coils for freeze
drying, a screw extruder and die, a chimney smoker, crushing mill rollers, and a
crosslinking clamp. Reclaimers look like disposal bins; conveyors have roller tracks.
The older compatibility kitchen also has a distinct stove drawing.

Type abbreviations no longer cover the factory equipment. Ingredient quantities,
food state and dock modes remain as small tags; hover/select still provides names,
settings and measured process changes. Colour reinforces the drawings, while their
shapes and mechanisms remain different without colour. Zooming reveals finer detail.

Real batches are visible through the equipment's chambers. Mixing paddles, pistons,
fans, separator rotors, mill rollers and plating arms animate while the actual
machine is processing and the clock runs. Idle, disabled and paused equipment does
not animate its mechanism. These drawings never create, move or transform food;
material transfer and molecular structure continue to follow the real simulation.
Machines, routes, recipes, stock and saves keep their existing model behaviour.

Visible food and molecular changes
----------------------------------
The floor now shows the actual food inside each machine and moving between its
ports. Ingredient colours mix with the measured input proportions. Feeder labels
identify their ingredient/dose, and doser labels identify the added agent.

Material chambers show fat droplets, air bubbles, gel networks and pearl shells,
ice crystals, browning, and distinct regions for separately plated components.
Homogenisation visibly shrinks fat droplets. Aeration increases bubbles; freezing
creates ice; setting builds networks. Warm/cold rims follow the food's actual
temperature, including while the batch travels on a conveyor.

Select or hover a machine to read its effect directly beside it on the floor.
The annotation compares the food when it entered with its current state: measured
input masses, added grams, temperature, relative droplet/particle size, air, gel,
shell and ice fractions, pH, protein/starch changes, browning and compound changes.
For batches already processing when a save opens, comparisons begin from the
restored state. Separators report the actual mass of both retained fractions. An empty device can
show its LAST measured effect; that label does not represent active food.

Only successful, adjacent, real batch transfers animate. Holding docks, disabled
lines and blocked outlets do not fabricate flowing food. Orange gates mark blocked
ports, including a separator's clockwise side outlet. Serving immediately removes
the consumed batch and any remaining transfer trace. The last successful transfer
can finish its brief visual movement after pausing; the clock and chemistry stay
paused. Higher run speeds shorten the transfer animation to match the tick rate.

Flow in the floor toolbar, or V, toggles transfer traces, route links and annotations.
Actual food chambers remain visible. Wheel zoom reveals finer structures. The floor
legend explains the visual symbols. All presentation data is read-only and temporary;
it creates no ingredients, advances no time and changes no saves or chemistry.
Droplet and particle sizes are relative game values, not micrometre measurements.
The structure graphics illustrate the stylised model rather than individual atoms.

A closer starting view
----------------------
Home opens a readable working area around the selected connected production line,
including nearby equipment. In the standard window, machine tiles are roughly
74 pixels wide, compared with the previous 38. Compact windows also start closer.
Long networks and remote construction do not force the opening camera to zoom out.
Select equipment and press Home to bring its local working area into view. F or Fit
shows the whole factory with a tighter margin. Wheel and pan still explore the
entire world, and the minimap shows every production district.

New factories contain a 91-machine demo network across a 16-by-13-tile district.
Its spherification, foam, freezing, emulsion and three-cut glass distillation
branches feed shared conveyor routes, a central mixer, homogeniser and final tasting
dock. The tails fraction also joins the shared finishing line. The opening camera
fits the whole network. Existing starter saves are interconnected in unused floor
space while keeping their live batches and records. Other custom factories keep
their saved layout and open around the selected production line.

The shared screen
-----------------
Left: searchable physical machines and 40 ingredients. The gram field accepts an
exact dose. Set source configures the selected feeder/doser/bath. Feeder and Doser
buttons place actual ingredient-handling machines on empty floor.

Centre: the persistent factory floor. Run, step, pan, zoom, build and connect
stages. The lower-left experiment cell operates alongside the sample production
lines. Machines, live batches and their movement are always visible.

Right: the selected machine's real inventory and settings. Batch, Second and Side
inspect actual input/output slots; Last inspects a served dock's read-only record.
Device settings scroll within the inspector. Naming and recorded dishes stay
available below. Recipe names inspect references; Build line places real equipment.

Controls
--------
Space run/pause; period step; R rotate; Q capture a plan; B saved plans.
V flow annotations; Home working area; F fit all; arrows pan; wheel smoothly zooms
around the pointer over the floor. Middle-drag, Alt-left-drag or right-drag pan;
right-click without dragging inspects a machine. Drag conveyors to paint lines.
Command/Ctrl-Z undo the last edit before the next simulation tick.
Command/Ctrl-S checkpoint; Command/Ctrl-L restore. Escape cancels a tool/menu.
Tab cycles text fields, never changes the view. Typing captures shortcuts.
Pause the clock to retain construction undo while comparing experiments.

All food creation and changes occur through the floor's actual devices. Temporary
plan, request and help menus do not replace the factory view.

Open sandbox
------------
A sparse world expands to +/-1,000,000 tiles. All factory machines and ingredients are available immediately. Free Build keeps pantry purchases and construction
free, while the factory itself still needs finite input buffers: electricity powers
processing, process water is drawn by machines that need it, Produce and Dairy
ingredients consume farm produce, and Agent ingredients consume chemicals.
Factory Inputs in the inspector gives direct deliveries. Deliveries are free in
Free Build and priced in Economy. Farms, power generation, water works and
chemical production are not modeled yet. The starting buffers are 10,000
electricity units, 150 water units, 50 kg of farm produce and 2.5 kg of chemicals.
Deliveries add 2,500 electricity, 100 water, 25 kg produce or 2.5 kg chemicals;
Economy prices are shown on the inspector controls. Optional requests pay bonuses
and never gate invention. Switch economy off at any time. The sample factory also
contains pearl, citrus-air, frozen-cream and emulsion production lines.

Physical devices: conveyors, feeders, mixers, setting cabinets, aerators, blast
chillers, thermal reactors, dehydrators, aroma vaults, thermal cyclers, tasting
docks, reclaimers, precision dosers, homogenisers, spherification baths, sous vide
baths, fermenters, enzyme incubators, centrifuges, filters, vacuum chambers,
 freeze dryers, extruders, component platers, smokers, colloid mills, binders, glass tubing, residence coils, stopcocks, receiving
beakers, heated flasks, cooling condensers, burettes, glass mixing junctions,
fractionating stills, reflux columns and fraction receivers.

Ingredients: syrup, brine, broth, citrus, cream, cocoa, water, milk, egg white,
yolk, butter, olive oil, coconut milk, tomato, strawberry, beet, mushroom, coffee,
tea, vanilla, wine, honey, glucose, sucrose, salt, alginate, calcium lactate, agar,
gelatin, pectin, lecithin, starch, amylase, papain, lactic culture, yeast, citrate,
bicarbonate, transglutaminase and xanthan.

Molecular model
---------------
Actual batches track masses in 25 compound groups, pH/buffering, water activity,
aromas and process history. Calcium/alginate crosslinking, agar/gelatin hydration
and setting, protein denaturation/curdling/binding, starch gelatinisation and
amylase, proteolysis, culture/yeast metabolism, heat inactivation, emulsification,
foam drainage, freezing-point depression, crystal growth, evaporation, Maillard
browning and caramelisation interact as food passes through the real machines.

Reactions and separation conserve tracked material. Evaporated water and vented
gas are counted explicitly. Split branches retain both fractions and shared
ancestry is deduplicated on recombination. Recorded physical routes include
both inputs to a mixer or plater, as well as useful side-output handling.

This is a stylised game model. Grouped compounds, pH estimates, thresholds,
quality and accelerated ticks are designed for play, rather than validated
predictions or real food preparation times. Concept references consulted:
  https://sciencecooking.seas.harvard.edu/
  https://doi.org/10.1088/0031-9120/49/5/512
  https://pubs.acs.org/doi/10.1021/ed076p624

Saves and older games
--------------------
Autosave: factory.json. Manual checkpoint: factory-checkpoint.json (separate).
Save format 3 preserves actual batches, routes, machine modes, latest servings,
recorded dishes and their physical layouts, outputs, blueprints and economy.
Loaded factories are paused. Versions 1 and 2 remain readable.

An older off-floor draft is recovered once into a real holding tasting dock near
7, 2, preserving its existing material. Older recipe references remain readable;
without a recorded physical line they cannot spawn food or construct a complete
line. Recreate their stages with machines and record the actual result instead.
Old programmable kitchens remain supported to preserve existing factories, but
are unavailable for new construction or new dish workflows. Plans containing
those compatibility devices cannot be stamped through the new interface.
Older notebook files, the supplied HTML and browser localStorage are untouched.

macOS: ~/Library/Application Support/Aftertaste/
Windows: %APPDATA%/Aftertaste/
Linux: $XDG_DATA_HOME/aftertaste/ or ~/.local/share/aftertaste/
AFTERTASTE_DATA_DIR overrides the save directory. QA uses isolated build folders.

Build and verification
----------------------
Requires C++17 and CMake 3.20+. All dependencies are vendored: raylib 5.5 and
nlohmann/json 3.11.3. No download is needed. Run ./build.sh, or:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release -j
  ctest --test-dir build -C Release --output-on-failure

For standalone tests add -DAFTERTASTE_BUILD_APP=OFF. For sanitizers also add
-DAFTERTASTE_SANITIZE=ON in a separate build. On this macOS runtime use
ASAN_OPTIONS=detect_leaks=0 when running sanitizer tests.

The supplied app is arm64 macOS. Windows/Linux builds use the same C++ source
(with normal raylib/X11 development libraries on Linux). System fonts are used
when available, with a built-in fallback. No web view, JavaScript engine, server,
network resource or development checkout is required to run the application.

Native verification:
  Aftertaste.app/Contents/MacOS/Aftertaste --floor-test
  Aftertaste.app/Contents/MacOS/Aftertaste --glass-test
  Aftertaste.app/Contents/MacOS/Aftertaste --machine-test --size 1380x810
  Aftertaste.app/Contents/MacOS/Aftertaste --flow-test
  Aftertaste.app/Contents/MacOS/Aftertaste --flow-closeup
Capture with --screenshot /absolute/path.png; also --size WIDTHxHEIGHT,
--fresh-factory and --factory-ticks N. Older test and launch aliases use the same
floor interface. workspace-preview.png shows the actual running native game.

Sources: Interface.* (native helpers), FactoryInterface.cpp (floor controls/input), MachineArt.* (scalable, text-free equipment silhouettes and mechanisms),
GlassArt.* (transparent tube walls, joints, bends, coils and path sampling),
FloorRendering.cpp (live material
chambers, actual transfer animation and on-floor annotations), FloorTelemetry.*
(read-only entry/output measurements and batch tracking),
WorkspacePanels.cpp (library and live inspector), Factory.* (production/storage),
FactoryFloor.cpp (physical experimentation, routes, dish recording and layouts),
Culinary.* (material model). The original reactor engine remains a tested numerical
reference in Model.*, Simulation.cpp, Scoring.cpp, Forecast.cpp and Persistence.cpp.

Licenses
--------
raylib: Ramon Santamaria and contributors, zlib (vendor/raylib/LICENSE).
JSON for Modern C++: Niels Lohmann and contributors, MIT (vendor/json-LICENSE.txt).
The supplied game's content retains its original ownership.
