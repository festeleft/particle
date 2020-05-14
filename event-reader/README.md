Format for Data Stream

{
  "data": "{SEE BELOW}",
  "ttl":60,
  "published_at":"2018-01-31T01:27:41.249Z",
  "coreid":"57002c000b51353432383931",
  "name": "com-cranbrooke-observations"
}


{
"data": {
    "first": <time>,
    "last": <time>,
    "sample-count": <int>,
    "samples": [{
        "metric": 'String',
        "unit": 'String',
        "first": <float>,
        "last": <float>,
        "min": <float>,
        "max": <float>
        "average": <float>,
        "alert": Bool        
      },
      { ... }
    ],
    "counters": [{
      "name": 'String',
      "first": <int>,
      "last": <int>,
      "alert": Bool        
    }]
    "booleans": [{
      "name": 'String',
      "first": Bool,
      "last": Bool,
      "alert": Bool        
    }]  
}

Boolean examples:
  buttonPressed
  switchClosed

Counter Examples:
  uptime

Sample Examples:
  temperature
  humidity
  luminosity
