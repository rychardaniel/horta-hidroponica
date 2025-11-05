package com.hortahidroponica.model;

// Não precisa mais do @JsonIgnore aqui
import io.quarkus.hibernate.orm.panache.PanacheEntityBase;
import jakarta.persistence.*;

@Entity
@Table(name = "\"HORTA\"")
public class Horta extends PanacheEntityBase {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "\"ID\"")
    public Integer id;

    @Column(name = "\"NOME\"", length = 100, nullable = false)
    public String nome;

    @Column(name = "\"STATUS\"", nullable = false)
    public Integer status = 1;

    @ManyToOne(fetch = FetchType.LAZY, optional = true)
    @JoinColumn(name = "\"ID_PARAMETRO\"", nullable = true)
    public Parametro parametro;
}