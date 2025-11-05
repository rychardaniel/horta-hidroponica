package com.hortahidroponica.model;

import com.fasterxml.jackson.annotation.JsonBackReference; // Importe esta classe
import com.fasterxml.jackson.annotation.JsonIgnore;
import io.quarkus.hibernate.orm.panache.PanacheEntityBase;
import jakarta.persistence.*;

import java.math.BigDecimal;

@Entity
@Table(name = "\"PARAMETRO\"")
public class Parametro extends PanacheEntityBase {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "\"ID\"")
    public Integer id;

    @Column(name = "\"NOME\"", length = 100, nullable = false)
    public String nome;

    @Column(name = "\"T_BOMBA_MINUTOS\"", precision = 5, scale = 2, nullable = false)
    public BigDecimal tempoBombaMinutos;

    @Column(name = "\"T_INTERVALO_MINUTOS\"", precision = 5, scale = 2, nullable = false)
    public BigDecimal tempoIntervaloMinutos;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "\"ID_CULTURA\"", nullable = false)
    @JsonBackReference
    public Cultura cultura;
}