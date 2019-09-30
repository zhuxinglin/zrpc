package xml_config


import (
    "net/http"
    "shmconfig"
    "errors"
    "time"
    "io/ioutil"
    "encoding/json"
)

const pszUri = "/xml/config/query?key=[";

type XmlConfigResp struct
{
    key string;
    value string;
};

type ConfigResp struct
{
    data []XmlConfigResp;
};

func getUrl(key* []string) (string, error) {
    xml_addr := shmconfig.GetValue("xml.config.server.addr", "");
    if xml_addr == "" {
        return "", errors.New("get xml server addess failed")
    }

    var url string = "http://" + xml_addr + pszUri;
    for k, v := range *key {
        if (k == 0){
            url += "\"" + v + "\"";
        }else{
            url += ",\"" + v + "\"";
        }
    }
    url += "]";
    return url, nil;
}

func Query(key* []string) (map[string]string, error) {
    url, err := getUrl(key);
    if err != nil {
        return nil, err;
    }

    cli := http.Client{Timeout: 3 * time.Second};
    resp, err := http.NewRequest("GET", url, nil);
    if (err != nil) {
        return nil, err;
    }

    resp.Header.Add("Connection", "keep-alive");
    response, err := cli.Do(resp);
    if (err != nil) {
        return nil, err;
    }

    defer response.Body.Close();

    body, err := ioutil.ReadAll(response.Body);
    if err != nil {
        return nil, err;
    }

    var con ConfigResp;
    json.Unmarshal(body, &con);

    var xml map[string]string = make(map[string]string)
    for _, v := range con.data {
        xml[v.key] = v.value;
    }
    return xml, nil;
}
