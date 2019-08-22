create table xml_config(
    id bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '主键',
    xml_key1 VARCHAR(64) NOT NULL DEFAULT '' COMMENT '配置KEY1',
    xml_key2 VARCHAR(64) NOT NULL DEFAULT '' COMMENT '配置KEY2',
    xml_key3 VARCHAR(64) NOT NULL DEFAULT '' COMMENT '配置KEY3',
    xml_key4 VARCHAR(64) NOT NULL DEFAULT '' COMMENT '配置KEY4',
    xml_value TEXT NOT NULL DEFAULT '' COMMENT '配置值',
    status int(11) unsigned NOT NULL DEFAULT '0' COMMENT '状态,0为草搞,1为编辑,2为发布',
    del_flag int(11) unsigned NOT NULL DEFAULT '0' COMMENT '删除状态',
    create_time bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
    update_time bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '最后更新时间',
    author varchar(64) NOT NULL DEFAULT '' COMMENT '操作者',
    description varchar(512) NOT NULL DEFAULT '' COMMENT '描述',
    PRIMARY KEY (id),
    UNIQUE KEY seq_key (xml_key1,xml_key2,xml_key3,xml_key4)
)ENGINE=InnoDB AUTO_INCREMENT=100000 DEFAULT CHARSET=utf8;

