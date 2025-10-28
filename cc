error: virtualservices.networking.istio.io "k811-loan-leads-service" could not be patched: admission webhook "validation.istio.io" denied the request: configuration is invalid: HTTP route, redirect or direct_response is required
You can run `kubectl replace -f /tmp/kubectl-edit-216926021.yaml` to try this update again.


i did below

# Please edit the object below. Lines beginning with a '#' will be ignored,
# and an empty file will abort the edit. If an error occurs while saving this file will be
# reopened with the relevant failures.
#
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  annotations:
    meta.helm.sh/release-name: uat-k811-loan-leads-service
    meta.helm.sh/release-namespace: loan-leads-uat
  creationTimestamp: "2025-03-04T11:33:02Z"
  generation: 4
  labels:
    app.kubernetes.io/managed-by: Helm
  name: k811-loan-leads-service
  namespace: loan-leads-uat
  resourceVersion: "376042269"
  uid: 0faa11c9-f654-4143-9992-d5f5afed669f
spec:
  gateways:
  - loan-leads-gateway
  hosts:
  - '*'
  http:
  - corsPolicy:
      allowOrigins:
      - exact: https://loan.uat.koatk811.com
      - exact: https://loan.uat.kotak811.bank.in
  - headers:
      response:
        set:
                Access-Control-Allow-Origin: https://loan.uat.kotak811.com, https://loan.uat.kotak811.bank.in
    match:
    - uri:
        prefix: /lls
    route:
    - destination:
        host: k811-loan-leads-service
        port:
          number: 8080
      weight: 100
