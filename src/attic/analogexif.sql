CREATE TABLE MetaTags (਀椀搀 䤀一吀䔀䜀䔀刀  倀刀䤀䴀䄀刀夀 䬀䔀夀 䄀唀吀伀䤀一䌀刀䔀䴀䔀一吀Ⰰ 
TagName TEXT  NOT NULL,਀吀愀最吀攀砀琀 吀䔀堀吀  一伀吀 一唀䰀䰀Ⰰ 
PrintFormat TEXT DEFAULT '%1' NOT NULL,਀吀愀最吀礀瀀攀 一唀䴀䔀刀䤀䌀 䐀䔀䘀䄀唀䰀吀 　 一伀吀 一唀䰀䰀Ⰰ 
Flags NUMERIC DEFAULT 0 NOT NULL,਀䄀氀琀吀愀最 吀䔀堀吀 一唀䰀䰀Ⰰ 
)਀ 
CREATE TABLE Settings (਀椀搀 䤀一吀䔀䜀䔀刀  倀刀䤀䴀䄀刀夀 䬀䔀夀 一伀吀 一唀䰀䰀Ⰰ 
SetId INTEGER  NOT NULL,਀匀攀琀嘀愀氀甀攀 一唀䴀䔀刀䤀䌀  一唀䰀䰀Ⰰ 
SetValueText TEXT  NULL਀⤀㬀 
਀䌀刀䔀䄀吀䔀 吀䄀䈀䰀䔀 䜀攀愀爀吀攀洀瀀氀愀琀攀 ⠀ 
id INTEGER PRIMARY KEY AUTOINCREMENT,਀䜀攀愀爀吀礀瀀攀 一唀䴀䔀刀䤀䌀 䐀䔀䘀䄀唀䰀吀 　 一伀吀 一唀䰀䰀Ⰰ 
TagId INTEGER REFERENCES MetaTags(id),਀伀爀搀攀爀䈀礀 一唀䴀䔀刀䤀䌀 䐀䔀䘀䄀唀䰀吀 　 一伀吀 一唀䰀䰀 
);਀ 
CREATE TABLE UserGearItems (਀椀搀 䤀一吀䔀䜀䔀刀  倀刀䤀䴀䄀刀夀 䬀䔀夀 䄀唀吀伀䤀一䌀刀䔀䴀䔀一吀Ⰰ 
ParentId INTEGER REFERENCES UserGearItems(id),਀䜀攀愀爀吀礀瀀攀 一唀䴀䔀刀䤀䌀 䐀䔀䘀䄀唀䰀吀 　 一伀吀 一唀䰀䰀Ⰰ 
GearName TEXT  NOT NULL,਀伀爀搀攀爀䈀礀 一唀䴀䔀刀䤀䌀 䐀䔀䘀䄀唀䰀吀 　 一伀吀 一唀䰀䰀 
);਀ 
CREATE TABLE UserGearProperties (਀椀搀 䤀一吀䔀䜀䔀刀  倀刀䤀䴀䄀刀夀 䬀䔀夀 䄀唀吀伀䤀一䌀刀䔀䴀䔀一吀Ⰰ 
GearId INTEGER  REFERENCES UserGearItems(id),਀吀愀最䤀搀 䤀一吀䔀䜀䔀刀  刀䔀䘀䔀刀䔀一䌀䔀匀 䴀攀琀愀吀愀最猀⠀椀搀⤀Ⰰ 
TagValue TEXT  NULL,਀䄀氀琀嘀愀氀甀攀 吀䔀堀吀 一唀䰀䰀Ⰰ 
OrderBy NUMERIC DEFAULT 0 NOT NULL਀⤀㬀 
