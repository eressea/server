#!/bin/bash

box-upload-id() {
SRC="$1"
DST="$2"
FOLDER_ID="$3"
box files:upload -q -p "$FOLDER_ID" -n "$DST" "$SRC"
}

box-update-id() {
SRC="$1"
FILE_ID="$2"
box files:versions:upload -q "$FILE_ID" "$SRC"
}

box-file-id() {
  FOLDER_ID="$1"
  FILE="$2"
  box folders:get --json \
	  "$FOLDER_ID" | jq ".item_collection.entries[] | select(.name==\"$FILE\") | .id" | tr -d \"
}

box-upload() {
  SRC="$1"
  DST=$(basename "$SRC")
  FOLDER_ID=2654558997
  FILE_ID=$(box-file-id "$FOLDER_ID" "$DST")
  if [ -z "$FILE_ID" ] ; then
    echo upload
    box-upload-id "$SRC" "$DST" "$FOLDER_ID"
  else
    echo update
    box-update-id "$SRC" "$FILE_ID"
  fi
}

