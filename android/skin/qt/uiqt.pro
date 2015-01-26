TEMPLATE = subdirs
DIRLIST = build32 build64
for(dir, DIRLIST): exists($$dir): exists($$dir/*.pro): SUBDIRS += $$dir