# Research baseline: Latin Europe around 1200

Status: RESEARCH BASELINE v1  
Reference window: roughly **1180–1230**  
Scope: comparative Latin/Western-Central European baseline, not a claim that one social system covered all of Europe.

This document records the historical evidence that should shape Simulation models before magic is applied. It is not gameplay content and it is not a universal medieval template.

A concrete scenario must still state its region, settlement type, climate/geography and political context. Practices varied strongly by region. When a mechanic needs a precise rate, obligation, crop yield, price, legal rule or institutional form, research that local mechanic separately instead of extrapolating a Europe-wide constant from this overview.

## Why circa 1200

The project uses the decades around 1200 as the non-magical reference horizon because they sit in the High Middle Ages after major eleventh–twelfth-century changes in rural settlement, church organization, law, towns and trade, but before later-medieval shocks such as the fourteenth-century Black Death.

The baseline is therefore a **counterfactual starting point**, not a requirement that the magical world reproduce European history after 1200. Once magic exists, history is expected to diverge causally.

## Findings that matter to the simulation

### 1. Do not model a universal feudal pyramid

A single `king -> great lord -> knight -> serf` hierarchy is too rigid to serve as the world model.

Susan Reynolds' survey of government and community emphasizes that historians themselves debate whether `lordship` or `government` better describes power in the eleventh and twelfth centuries. Richard Britnell warns that the compact manor with one lord, demesne and attached unfree tenants is overrepresented by surviving records and did not describe most land even within Britain and Ireland. Recent work on multiple lordship likewise stresses overlapping dependencies and the historiographical problems of treating `feudalism` as one precise universal system.

**Model consequence:** authority must be represented through concrete relations — offices, jurisdiction, land/control rights, obligations, patronage, membership, coercive capacity, legal status and recognized claims — rather than one global feudal rank.

### 2. Rural production is central, but peasants are agents rather than scenery

The New Cambridge Medieval History treats rural economy and demographic growth as a central process of the eleventh and twelfth centuries. More recent social history also treats peasants as decision-makers who traded, migrated, participated in communities and changed their livelihoods rather than as passive units attached permanently to one lord.

**Model consequence:** early economic slices should begin with land/access, labor/time, household stores, production, obligations/extraction, transport and consumption. Ordinary actors need feasible choices inside their constraints; `Peasant` must not be an immutable behavioral class.

### 3. Markets and towns are already important around 1200

Derek Keene describes vigorous growth of European commerce and urban life over the eleventh and twelfth centuries, with strong regional variation and connections to Mediterranean and longer-distance networks. Hunt and Murray describe medieval business as developing between peasant agriculture and powerful lordship, with organizational tools, credit and institutions evolving around opportunities and constraints.

**Model consequence:** do not model the period as pure self-sufficient barter villages. Markets, money, credit, craft specialization, transport and merchant organization can matter, while exchange remains embedded in rights, coercion, institutions and physical stock.

### 4. Economic power and coercive/institutional power interact

Research on medieval markets emphasizes that unfree status, land concentration, lordship and coercive extraction coexisted with market exchange. Lords — secular and ecclesiastical — could redirect surplus; rulers and institutions could tax, demand service, regulate or protect exchange.

**Model consequence:** economy cannot be a detached global price engine. Ownership/control, access, jurisdiction, rents/taxes/dues, protection, transport risk and institutional power may alter prices and opportunities.

### 5. Political order is polycentric and personal/institutional at the same time

Around this period kingship, aristocratic lordship, ecclesiastical institutions, towns/communes and local communities could all exercise meaningful authority. Government was not a modern unitary bureaucracy. Royal power depended on resources, recognized office, law/custom, counsel, allies, local officeholders and the ability to enforce decisions.

**Model consequence:** political power should emerge from several resources and relationships. A ruler is not powerful because an entity has `rank = king`; an office matters because actors/institutions recognize it, attach rights/obligations to it and can enforce or contest those claims.

### 6. The Church is a first-class economic, political and social institution

The institutional church around 1200 was not merely religious flavor. Cambridge's treatment of the church from 1073–1216 describes a developed hierarchy and delegated authority. Oxford scholarship on the church as lord emphasizes ecclesiastical institutions as major property holders whose lordships interacted with peasants, monarchies and towns.

**Model consequence:** where the setting has an analogous organized religion, it may own/control land, provide offices, education, welfare, legitimacy, law/jurisdiction, patronage and political leverage. Do not reduce religion to an NPC belief stat if institutions materially exist.

### 7. Social position has several axes

Social historians caution against treating medieval society as stratified by one universal class/order dimension. Status could depend on birth, legal freedom/unfreedom, wealth, occupation, office, urban membership, religious status, lordship, household/kin connections and other identities.

**Model consequence:** do not introduce one authoritative `SocialClass` or `NobleLevel` enum that decides all permissions. Use the smallest concrete state that a rule actually depends on: right, office, property, membership, obligation, reputation, relationship, skill, wealth, legal status, etc.

### 8. Mobility exists, but institutions and inherited advantage create friction

Alfred Haverkamp describes substantial geographical and social mobility in Western Europe in the eleventh–thirteenth centuries alongside continued defense of status by birth. Towns and professions created new opportunities without abolishing inherited hierarchy. Other work on peasant status around 1200 shows that legal subordination could also become more sharply defined in some regions.

**Model consequence:** upward or downward mobility must be possible through real paths, but it should encounter actual barriers: property/access, patronage, education/knowledge, office rules, legal status, networks, reputation, force and institutional resistance. These barriers are state/rules, not an arbitrary `max_rank_for_peasant` cap.

### 9. Law and jurisdiction are part of causal gameplay

Legal development around this era included renewed written legal traditions, canon law and multiple overlapping sources of authority. The player should not encounter one modern centralized criminal/legal API projected backward onto the setting.

**Model consequence:** legality, jurisdiction and enforcement must be separated. An action can violate one authority's rule while another authority cannot or will not enforce it. Rights and obligations should matter only where actors/institutions know, recognize and can act on them.

## Magic: how the historical baseline is used

The historical baseline answers:

> What constraints and institutions would plausibly exist without magic?

The magical simulation then asks:

> Which of those constraints does a concrete magical capability alter, who can access it, who notices, and how do actors/institutions adapt?

Magic must not be a decorative subsystem added after economy/social/politics are considered “finished”. It is a causal pressure that may alter the assumptions those systems rely on.

### Magic sensitivity surface

Every serious model should identify which of these broad causal surfaces it depends on and therefore could be changed by future magic:

- **health / mortality / fertility / work capacity**;
- **resource production / transformation / scarcity**;
- **transport / travel time / distance / communication**;
- **information / secrecy / verification / prediction**;
- **coercion / defense / violence / physical security**;
- **property / containment / access / theft prevention**;
- **environment / weather / water / soil / hazards**;
- **skill / learning / memory / capability acquisition**;
- **legitimacy / religion / recognized authority**;
- **institutional enforcement and countermeasures**.

This list is a review surface, **not** a magic taxonomy and not an enum that every spell must fit into.

A subsystem should not receive a meaningless global `magic_multiplier`. Concrete magic changes concrete domain facts or mechanisms.

### Example: healing magic

A healing capability might alter morbidity, mortality, recovery time or work capacity. Those changes can propagate into household labor, production, care burdens, population structure, wages/prices, military availability, religious authority and political legitimacy.

The correct implementation is not `economy.magic_bonus += 0.1`; it is to change the health-related causal state/rules and let dependent systems observe the resulting world state.

### Example: a magically powerful commoner

A commoner who acquires extraordinary coercive or productive capability can leap over normal material constraints. That does **not** mean the world applies an invisible anti-magic level cap.

Existing actors and institutions may instead react through mechanisms they actually possess:

- recruitment/patronage;
- alliance or marriage offers;
- licensing/office grants;
- surveillance and information gathering;
- religious/legal legitimation or condemnation;
- monopoly/cartel attempts;
- taxation/extraction;
- counter-magic or physical defenses;
- coalition formation;
- bribery, assassination, exile or war.

If those mechanisms are insufficient, the established order can genuinely fail. “The world resists” means institutions pursue their interests with available capabilities; it does not mean rulers receive plot armor.

## Adaptive causal fidelity

The simulation should be as detailed as the **causal question and gameplay exposure require**, not uniformly microscopic.

This follows standard simulation practice: abstraction is necessary, and model entities/state/processes should be chosen for the model's purpose. Multi-level agent-based simulation research also supports coupling different abstraction levels when different detail is useful.

### Resolution is based on causal relevance, not camera distance

An entity/process deserves identity-resolved simulation when one or more are true:

- the player can currently interact with it;
- a named/persistent identity or relationship matters to future gameplay;
- it participates in an unresolved causal chain that can reach player-facing state;
- individual heterogeneity changes the relevant outcome;
- the result must be explainable through events/history;
- a mechanic explicitly exposes that entity through a projection.

Something can remain important while offscreen. A distant ruler, debt, epidemic, caravan, market shortage or political decision can stay causally active even when Godot has no node for it.

### Three conceptual representation levels

These are **modeling choices**, not runtime classes that must be implemented immediately:

1. **Identity-resolved** — persistent individual actors/items/institutions whose own state/choices/history matter.
2. **Aggregate-resolved** — stocks, populations or distributions where totals/composition/flows matter but individual identity does not yet affect gameplay.
3. **Deferred/derived** — detail that is not stored or ticked because no current mechanic depends on it; compute or introduce it only when a real causal need appears.

Do not create a generic dynamic-resolution framework before a mechanic requires promotion/demotion between these levels.

### Promotion/demotion invariants

When later mechanics require changing representation fidelity, the transition must preserve the facts that matter:

- conserved stocks/counts/resources;
- existing named or historically consequential identities;
- ownership/rights/debts/obligations that affect gameplay;
- already-observed history and causal outcomes;
- distributions needed to avoid impossible disaggregation;
- deterministic/replay requirements.

Do not merge away a named merchant who owes the player money just because the player walked out of town. Do not materialize ten convenient villagers later if the aggregate state says the settlement lost most of its population.

### What not to simulate

Do not store/tick a detail merely because real humans possess it.

Examples that normally remain absent until a mechanic needs them:

- complete genetics;
- every meal and bodily process;
- private memories that cannot affect behavior;
- exact household objects with no ownership/use/trade consequence;
- every distant person's minute-by-minute schedule;
- decorative social facts with no future opportunity/cost/behavior effect.

Realism means preserving the causal structures that produce believable observable outcomes, not maximizing variable count.

## Source set

The sources below are load-bearing starting points; feature-specific research should add narrower regional/mechanic evidence.

1. David Luscombe & Jonathan Riley-Smith (eds.), *The New Cambridge Medieval History, Vol. IV: c.1024–c.1198*, especially:
   - Robert Fossier, “The Rural Economy and Demographic Growth”, DOI `10.1017/CHOL9780521414104.003`;
   - Derek Keene, “Towns and the Growth of Trade”, DOI `10.1017/CHOL9780521414104.004`;
   - Susan Reynolds, “Government and Community”, Cambridge University Press;
   - Peter Landau, “The Development of Law”, Cambridge University Press;
   - I. S. Robinson, “The Institutions of the Church, 1073–1216”, DOI `10.1017/CHOL9780521414104.012`.
2. Richard Britnell, “Social bonds and economic change”, in *The Twelfth and Thirteenth Centuries*, Oxford University Press, DOI `10.1093/oso/9780198731405.003.0004`.
3. Hannah Boston, *Lordship and Locality in the Long Twelfth Century* (2024), introduction, DOI `10.1017/9781805431718.001`.
4. George Dameron, “The Church as Lord”, in *The Oxford Handbook of Medieval Christianity*, DOI `10.1093/oxfordhb/9780199582136.013.028`.
5. Alfred Haverkamp, *Medieval Germany 1056–1273*, ch. 8 “Social Change”, Oxford University Press, DOI `10.1093/acprof:oso/9780198221722.003.0009`.
6. Edwin S. Hunt & James Murray, *A History of Business in Medieval Europe, 1200–1550*, Cambridge University Press, especially DOI `10.1017/CBO9780511626005.003` and `10.1017/CBO9780511626005.004`.
7. S. H. Rigby, “Social structure and economic change”, in *A Social History of England, 1200–1500*, DOI `10.1017/CBO9781139167154.002`.
8. Christopher Dyer, *Peasants Making History: Living in an English Region 1200–1540*, Oxford University Press (2022), as a region-specific corrective to passive-peasant models.
9. Peer-Olaf Siebers & Uwe Aickelin, “Introduction to Multi-Agent Simulation” (2008), DOI `10.4018/978-1-59904-843-7.ch062`, University of Nottingham repository.
10. Volker Grimm et al., “The ODD protocol for describing agent-based and other simulation models: A second update to improve clarity, replication, and structural realism” (2020), DOI `10.18564/jasss.4259`.
11. Philippe Mathieu, Gildas Morvan & Sébastien Picault, “Multi-level agent-based simulations: Four design patterns”, *Simulation Modelling Practice and Theory* 83 (2018), DOI `10.1016/j.simpat.2017.12.015`.

## Uncertainty and falsifiers

This baseline should be revised when:

- the chosen world region requires a materially different institutional/economic baseline;
- a narrower historical study contradicts a broad generalization used by a mechanic;
- a game mechanic makes a previously irrelevant distinction causally important;
- magic changes a load-bearing constraint enough that the inherited historical assumption no longer reproduces plausible downstream behavior;
- long-horizon tests produce trajectories that cannot be explained from the stated mechanisms.

Historical authority is evidence for the **non-magical starting model**. Once the world diverges under magic, internal causal consistency and observable consequences matter more than forcing outcomes back toward real chronology.
