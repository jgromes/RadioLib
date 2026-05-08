#!/bin/bash

grep -zaoP 'RadioLib Info[\x20-\x7E\s]*' "$1"
echo ""
