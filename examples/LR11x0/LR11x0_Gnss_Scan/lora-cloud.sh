#! /usr/bin/bash -e
# Parse the output of the sketch and send the NAV data to LoRaCloud to get the position
# Uses `curl`` to perform the request, `date`` to convert unix timestamp to readable date, and
# `bc` to calculate real error
# This script was developed on Linux, adapting it to MacOS or Windows will require some changes to paths
# For the auth tokens, create a .lora-cloud.auth file assigning the AUTH_TOKEN variable

SERIAL=$(echo /dev/serial/by-id/usb-Seeed_Wio_Tracker_1110_*)
BAUD=115200

# URL="https://gls.loracloud.com/api/v3/solve/gnss_lr1110_singleframe" # deprecated
URL="https://mgs.loracloud.com/api/v1/solve/gnss_lora_edge_singleframe"
source lora-cloud.auth

# haversine formula for distance
function haversine() {
    local lat1=$1
    local lon1=$2
    local lat2=$REAL_LAT
    local lon2=$REAL_LON
    local expr="
      torad = 3.141593/180
      avglat = s(($lat1- $lat2)*torad/2)
      avglon = s(($lon1- $lon2)*torad/2)
      a = avglat*avglat + c($lat1*torad)*c($lat2*torad)*avglon*avglon
      dist = 12742000 * a(sqrt(a) / sqrt(1-a)) *3
      scale=0
      dist/3
    "
    # echo "  Expr: $expr"m
    echo "  Real error: $(bc -l <<<"$expr")m"
}

# query LoRaCloud
function query() {
    local data=$1
    local time=$2
    local accuracy=$3
    echo "LoRaCloud:"
    # add gnss_capture_time to body if available
    if [[ -n "$time" ]]; then
        # LoRaCloud is not explicit about when to measure the acquisition time in the device
        # Specifying an accuracy <16s causes it to use the specified time as initial estimate
        # Lower values causes it to reject solutions outside that error bound
        # Higher values causes it to use the time encoded in the payload (16s granularity)
        # Best is probably accuracy=15 or omitting this altogether
        # Note that the docs are self-contradictory, saying that the default time is the server's...
        local body='{"payload":"'$data'",
            "gnss_capture_time":'$time',"gnss_capture_time_accuracy":15}'
            # "gnss_assist_position":'$ASSIST_POS'}'
    else
        local body='{"payload":"'$data'"}'
    fi
    local header=="Accept: application/json;Ocp-Apim-Subscription-Key: $AUTH_TOKEN;"
    local res=$(curl -s -S -d "$body" \
        -HAccept:application/json -HOcp-Apim-Subscription-Key:$AUTH_TOKEN \
        $URL)
    # echo "  Result: $res"
    local POS_RE='"llh":\[([-0-9.]*),([-0-9.]*),([-0-9.]*)'
    local GPS_RE='"capture_time_gps":([0-9.]*)'
    local UTC_RE='"capture_time_utc":([0-9.]*)'
    local ACC_RE='"accuracy":([0-9.]*)'
    local ERR_RE='"errors": *[[](.*)]'
    if [[ "$res" =~ $ERR_RE ]]; then
        echo "  *** Location error: ${BASH_REMATCH[1]}"
        # echo "  Request body: $body"
    else
        if [[ "$res" =~ $POS_RE ]]; then
            local lat=${BASH_REMATCH[1]}
            local lon=${BASH_REMATCH[2]}
            local alt=${BASH_REMATCH[3]}
            echo "  Position: $lat $lon alt: $alt"
            haversine $lat $lon
        fi
        if [[ "$res" =~ $ACC_RE ]]; then
            local acc=${BASH_REMATCH[1]}
            if [[ "$acc" = 0* ]]; then echo "  Claimed accuracy: unknown"; else echo "  Claimed accuracy: ${acc}m"; fi
        fi
        if [[ "$res" =~ $GPS_RE ]]; then
            local gps=${BASH_REMATCH[1]}
            local delta=$(( ${gps%.*} - $time )) # note: integer only...
            echo "  GPS time: $gps -- delta=${delta}s" # delta to what we submitted in the request
        fi
        if [[ "$res" =~ $UTC_RE ]]; then
            local txt=$(date -u -Iseconds -d "@${BASH_REMATCH[1]}")
            echo "  UTC time: ${BASH_REMATCH[1]} -- $txt"
        fi
    fi
}

RE='^NAV( @([0-9]*))?: *([0-9a-zA-Z]*)' # regular expression for NAV line output by sketch

stty -F$SERIAL 115200
while IFS=\b read -r line; do
    if [[ "$line" =~ $RE ]]; then
        echo "$line"
        time=${BASH_REMATCH[2]}
        data=${BASH_REMATCH[3]}
        query "$data" "$time"
    else
        echo "$line"
    fi
done <$SERIAL
