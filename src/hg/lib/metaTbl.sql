# metaTbl.sql was originally generated by the autoSql program, which also
# generated metaTbl.c and metaTbl.h.  This creates the database representation of
# an object which can be loaded and saved from RAM in a fairly
# automatic way.

#This contains metadata for a table, file or other predeclared object type.
CREATE TABLE metaTbl (
    objName varchar(255) not null,	# Object name or ID.
    objType     enum (
                 'table',               # Most objects are tables
                 'file'                 # some objects are files
               ) not null default 'table',
    var varchar(255) not null,	        # Metadata variable name.
    varType     enum (
                 'txt',                 # Most vars are txt
                 'binary'               # some vars are binary
               ) not null default 'txt',
    val longblob not null,	        # Metadata value.
              #Indices
    PRIMARY KEY(objName,var),
    UNIQUE(var,val(32),objName)
);