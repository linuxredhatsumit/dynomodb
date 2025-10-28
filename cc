spec:
  gateways:
  - loan-leads-gateway
  hosts:
  - '*'
  http:
  - corsPolicy:
      allowOrigins:
      - exact: https://loan.uat.kotak811.com
      - exact: https://loan.uat.kotak811.bank.in
    headers:
      response:
        set:
          Access-Control-Allow-Origin: "https://loan.uat.kotak811.com, https://loan.uat.kotak811.bank.in"
    match:
    - uri:
        prefix: /lls
    route:
    - destination:
        host: k811-loan-leads-service
        port:
          number: 8080
      weight: 100
