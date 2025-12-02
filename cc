locals {
  nonprod_dns_records = {
    apyuat = {
      name    = "apy.uat"
      content = "d2audzjm5us1na.cloudfront.net"
    }
    iipuat = {
      name    = "iip.uat"
      content = "d25y2b0tdik8yh.cloudfront.net"
    }
    ksecuat = {
      name    = "ksec.uat"
      content = "d1wxlhimn6abwc.cloudfront.net"
    }
    loanuat = {
      name    = "loan.uat"
      content = "da7s4ua62utue.cloudfront.net"
    }
    sccuat = {
      name    = "scc.uat"
      content = "d2d2u5vz2zez92.cloudfront.net"
    }
    ghiuat = {
      name       = "ghi.uat"
      content    = "checkpoint-np-alb-1879576181.ap-south-1.elb.amazonaws.com"
      owner_team = ["811-crosssell"]
    }
    passbookuat = {
      name       = "passbook.uat"
      content    = "checkpoint-np-alb-1879576181.ap-south-1.elb.amazonaws.com"
      owner_team = ["811-crosssell"]
    }
    agentkuat = {
      name       = "agent.uat"
      content    = "checkpoint-np-alb-1879576181.ap-south-1.elb.amazonaws.com"
      owner_team = ["811-crosssell"]
    }
    cckuat = {
      name       = "cc.uat"
      content    = "checkpoint-np-alb-1879576181.ap-south-1.elb.amazonaws.com"
      owner_team = ["811-crosssell"]
    }
    blportalkuat = {
      name       = "blportal.uat"
      content    = "checkpoint-np-alb-1879576181.ap-south-1.elb.amazonaws.com"
      owner_team = ["811-crosssell"]
    }
  }
  prod_dns_records = {
    www = {
      name    = "www"
      content = "811-checkpoint-nlb-054b5c6d3fe91b45.elb.ap-south-1.amazonaws.com"
    }
    iip = {
      name    = "iip"
      content = "d2drssdz0v01pg.cloudfront.net"
    }
    ksec = {
      name    = "ksec"
      content = "d27ljuqxuf2nw3.cloudfront.net"
    }
    loan = {
      name    = "loan"
      content = "d2u8zrp5rgaz21.cloudfront.net"
    }
    creditcard-on-fd = {
      name    = "creditcard-on-fd"
      content = "d22dyonzo4ekgl.cloudfront.net"
    }
    apy = {
      name    = "apy"
      content = "ds9enargjlzsj.cloudfront.net"
    }
    ghi = {
      name    = "ghi"
      content = "checkpoint-prod-alb-797480506.ap-south-1.elb.amazonaws.com"
    }
    cc = {
      name    = "cc"
      content = "checkpoint-prod-alb-797480506.ap-south-1.elb.amazonaws.com"
    }
    bl = {
      name    = "bl"
      content = "checkpoint-prod-alb-797480506.ap-south-1.elb.amazonaws.com"
    }
    sm = {
      name    = "sm"
      content = "d27d7qmzcxnt5n.cloudfront.net"
    }

    811apppurezento = {
        name    = "811apppurezento"
        content = "checkpoint-prod-alb-797480506.ap-south-1.elb.amazonaws.com"
    }
    agent = {
      name    = "811apppurezento"
      content = "checkpoint-prod-alb-797480506.ap-south-1.elb.amazonaws.com"
    }
  }

}

###################
module "nonprod_dns_records" {
  source     = "./cloudflare-modules/dns"
  for_each   = local.nonprod_dns_records
  name       = each.value.name
  type       = lookup(each.value, "type", "CNAME")
  content    = each.value.content
  owner_team = concat(lookup(each.value, "owner_team", []), ["811-team"])
}

module "prod_dns_records" {
  source     = "./cloudflare-modules/dns"
  for_each   = local.prod_dns_records
  name       = each.value.name
  type       = lookup(each.value, "type", "CNAME")
  content    = each.value.content
  owner_team = concat(lookup(each.value, "owner_team", []), ["811-team"])




   getting below error

   ame    = "cc"
module.records.route53_record_name
Missing key/value separator: Expected an equals sign ("=") to mark the beginning of the attribute value. If you intended to given an attribute name containing periods or spaces, write the name in quotes to create a string literal.
}
